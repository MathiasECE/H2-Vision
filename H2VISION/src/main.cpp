/**
 * ============================================================================
 * PROJET H2-VISION — FIRMWARE OFFICIEL DE TÉLÉMÉTRIE EMBARQUÉE (ESP32 / S3)
 * Compétition : Horizon Hydrogen Grand Prix (Catégorie Stock)
 * Équipe      : ECE Paris / Horizon Educational
 * ============================================================================
 * Architecture logicielle Dual-Core FreeRTOS :
 *   - Core 0 (50 Hz / 20 ms) : Acquisition déterministe des 4 capteurs + Loi de Faraday
 *   - Core 1 (10 Hz / 100 ms): Encapsulation trame binaire 20 octets + Émission LoRa SX1262
 */

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>

#include "config.h"
#include "telemetry_packet.h"
#include "sx1262_driver.h"
#include "web_dashboard.h"

// ============================================================================
// SERVEUR WEB EMBARQUÉ (POINT D'ACCÈS WI-FI H2-VISION)
// ============================================================================
WebServer webServer(80);

// ============================================================================
// INSTANCIATION DU MODULE RADIO LORA SX1262 (SPI)
// ============================================================================
SPIClass loraSPI(HSPI);
SX1262Driver radio(loraSPI, PIN_LORA_NSS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY);
bool lora_initialized = false;
bool ina226_initialized = false;

// ============================================================================
// VARIABLES D'INTERRUPTION MATÉRIELLE (PLACÉES EN RAM RAPIDE IRAM)
// ============================================================================

// --- 1. Capteur à Effet Hall A3144 (Vitesse & Régime Arbre) ---
volatile uint32_t hall_last_pulse_us = 0;
volatile uint32_t hall_period_us     = 0;
volatile uint32_t hall_pulse_count   = 0;

void IRAM_ATTR onHallPulseISR() {
    uint32_t now_us = micros();
    uint32_t dt_us  = now_us - hall_last_pulse_us;
    // Filtre anti-rebond : ignore les impulsions espacées de moins de 1000 µs
    if (dt_us >= HALL_DEBOUNCE_US) {
        hall_period_us     = dt_us;
        hall_last_pulse_us = now_us;
        hall_pulse_count++;
    }
}

// --- 2. Cordon Servo en Y (Capture Largeur d'Impulsion PWM Gâchette) ---
volatile uint32_t pwm_rise_time_us  = 0;
volatile uint32_t pwm_pulse_width_us = PWM_MIN_PULSE_US;
volatile uint32_t pwm_last_update_us = 0;

void IRAM_ATTR onThrottlePwmISR() {
    uint32_t now_us = micros();
    if (digitalRead(PIN_THROTTLE_PWM) == HIGH) {
        pwm_rise_time_us = now_us;
    } else {
        uint32_t width = now_us - pwm_rise_time_us;
        // Validation d'une impulsion RC standard (entre 800 µs et 2200 µs)
        if (width >= 800UL && width <= 2200UL) {
            pwm_pulse_width_us = width;
            pwm_last_update_us = now_us;
        }
    }
}

// ============================================================================
// PILOTE I2C NATIF POUR TEXAS INSTRUMENTS INA226
// ============================================================================
bool initINA226() {
    Wire.beginTransmission(INA226_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        return false; // Puce non détectée sur le bus I2C
    }
    // Configuration Register (0x00) :
    // Moyennage = 16 échantillons (bits 9-11 = 010)
    // Temps conversion Vbus = 1.1 ms (bits 6-8 = 100)
    // Temps conversion Vshunt = 1.1 ms (bits 3-5 = 100)
    // Mode = Shunt & Bus continu (bits 0-2 = 111) -> Valeur = 0x4527
    Wire.beginTransmission(INA226_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(0x45);
    Wire.write(0x27);
    return (Wire.endTransmission() == 0);
}

bool readINA226(float &voltage_v, float &current_a) {
    if (!ina226_initialized) {
        return false;
    }

    // 1. Lecture Tension Shunt (Registre 0x01, LSB = 2.5 µV)
    Wire.beginTransmission(INA226_I2C_ADDR);
    Wire.write(0x01);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)INA226_I2C_ADDR, (uint8_t)2) != 2) return false;
    int16_t raw_shunt = (Wire.read() << 8) | Wire.read();

    // 2. Lecture Tension Bus (Registre 0x02, LSB = 1.25 mV)
    Wire.beginTransmission(INA226_I2C_ADDR);
    Wire.write(0x02);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((uint8_t)INA226_I2C_ADDR, (uint8_t)2) != 2) return false;
    uint16_t raw_bus = (Wire.read() << 8) | Wire.read();

    float v_shunt_uv = (float)raw_shunt * 2.5f; // en microvolts
    voltage_v        = (float)raw_bus * 0.00125f; // en Volts
    current_a        = (v_shunt_uv * 1e-6f) / INA226_R_SHUNT_OHM; // Loi d'Ohm : I = U / R

    return true;
}

// ============================================================================
// LECTURE TEMPÉRATURE SONDE NTC 10k B3950 (LOI DE STEINHART-HART)
// ============================================================================
float readNtcTemperatureC() {
    // Moyennage sur 8 échantillons avec calibration millivolts interne ESP32
    uint32_t sum_mv = 0;
    for (int i = 0; i < 8; i++) {
        sum_mv += analogReadMilliVolts(PIN_NTC_ADC);
    }
    float v_out_mv = sum_mv / 8.0f;

    // Sécurité si sonde débranchée (court-circuit ou circuit ouvert)
    if (v_out_mv < 50.0f || v_out_mv > (ADC_VREF_MV - 50.0f)) {
        return 25.0f; // Valeur par défaut sécurisée
    }

    // Calcul de la résistance de la CTN dans le pont diviseur :
    // Montage : 3.3V -> NTC -> [GPIO_ADC] -> R_FIXED (10k) -> GND
    float r_ntc = NTC_R_SERIES_OHM * ((ADC_VREF_MV / v_out_mv) - 1.0f);

    // Équation de Steinhart-Hart (Paramètre Beta) :
    // 1/T = 1/T0 + (1/B) * ln(R / R0)
    float steinhart = log(r_ntc / NTC_R_NOMINAL_OHM) / NTC_BETA_COEFF;
    steinhart += 1.0f / (NTC_TEMP_NOMINAL_C + 273.15f);
    float temp_c = (1.0f / steinhart) - 273.15f;

    return temp_c;
}

// ============================================================================
// ÉTAT GLOBAL DU SYSTÈME & SYNCHRONISATION FREERTOS
// ============================================================================
struct SystemState {
    float voltage_v          = 0.0f;
    float current_a          = 0.0f;
    float power_w            = 0.0f;
    float energy_wh          = 0.0f;
    float h2_consumed_g      = 0.0f;
    float h2_remaining_pct   = 100.0f;
    float speed_kmh          = 0.0f;
    float rpm_shaft          = 0.0f;
    float distance_m         = 0.0f;
    float temp_stack_c       = 25.0f;
    uint8_t throttle_pct     = 0;
    uint16_t alerts_mask     = 0;
};

SystemState g_state;
SemaphoreHandle_t g_stateMutex;

// ============================================================================
// TÂCHE CORE 0 : ACQUISITION TEMPS RÉEL 50 HZ & CALCUL DE FARADAY
// ============================================================================
void taskAcquisition50Hz(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(ACQUISITION_PERIOD_MS);
    uint32_t last_step_us = micros();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        uint32_t now_us = micros();
        float dt_s = (now_us - last_step_us) * 1e-6f;
        last_step_us = now_us;

        // --- 1. Lecture Électrique (INA226) ---
        float v_meas = 0.0f, i_meas = 0.0f;
        bool ina_ok = readINA226(v_meas, i_meas);
        if (!ina_ok) {
            // Re-tentative de connexion I2C à chaud si débranché
            ina226_initialized = initINA226();
        }

        float p_meas = v_meas * i_meas;
        float d_energy_wh = (p_meas * dt_s) / 3600.0f;

        // --- 2. Intégration Loi de Faraday (Jauge Hydrogène) ---
        // Débit molaire H2 (mol/s) = I / (2 * F * rendement)
        float i_positive = (i_meas > 0.0f) ? i_meas : 0.0f;
        float dn_h2_mol  = (i_positive * dt_s) / (2.0f * FARADAY_CONSTANT * FARADAY_EFFICIENCY);
        float dm_h2_g    = dn_h2_mol * H2_MOLAR_MASS;

        // --- 3. Lecture Cinématique (Capteur Hall A3144) ---
        noInterrupts();
        uint32_t period_copy = hall_period_us;
        uint32_t last_pulse  = hall_last_pulse_us;
        uint32_t pulses_copy = hall_pulse_count;
        interrupts();

        float rpm = 0.0f;
        float speed_ms = 0.0f;
        if ((now_us - last_pulse) < HALL_TIMEOUT_US && period_copy > 0) {
            rpm = 60000000.0f / ((float)period_copy * MAGNETS_PER_REV);
            float wheel_rps = (rpm / 60.0f) / TRANSMISSION_RATIO;
            speed_ms = wheel_rps * PI * WHEEL_DIAMETER_M;
        }
        float wheel_circumference = PI * WHEEL_DIAMETER_M;
        float total_dist_m = ((float)pulses_copy / (MAGNETS_PER_REV * TRANSMISSION_RATIO)) * wheel_circumference;

        // --- 4. Lecture Température Pile (NTC 10k) ---
        float temp_c = readNtcTemperatureC();

        // --- 5. Lecture Gâchette Pilote (PWM Cordon Y) ---
        noInterrupts();
        uint32_t pwm_us = pwm_pulse_width_us;
        uint32_t pwm_age = now_us - pwm_last_update_us;
        interrupts();

        uint8_t throttle = 0;
        if (pwm_age < 200000UL) { // Signal PWM actif (< 200 ms)
            if (pwm_us <= PWM_NEUTRAL_PULSE_US) {
                throttle = 0; // Frein ou point mort
            } else {
                long pct = map((long)pwm_us, PWM_NEUTRAL_PULSE_US, PWM_MAX_PULSE_US, 0, 100);
                throttle = (uint8_t)constrain(pct, 0, 100);
            }
        }

        // --- 6. Mise à jour de l'état partagé avec protection Mutex ---
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            g_state.voltage_v      = v_meas;
            g_state.current_a      = i_meas;
            g_state.power_w        = p_meas;
            g_state.energy_wh     += d_energy_wh;
            g_state.h2_consumed_g += dm_h2_g;

            float rem_g = HYDROSTIK_CAPACITY_G - g_state.h2_consumed_g;
            g_state.h2_remaining_pct = constrain((rem_g / HYDROSTIK_CAPACITY_G) * 100.0f, 0.0f, 100.0f);

            g_state.rpm_shaft      = rpm;
            g_state.speed_kmh      = speed_ms * 3.6f;
            g_state.distance_m     = total_dist_m;
            g_state.temp_stack_c   = temp_c;
            g_state.throttle_pct   = throttle;

            // Évaluation des masques d'alertes
            uint16_t alerts = 0;
            if (temp_c >= ALERT_TEMP_WARN_C)             alerts |= ALERT_BIT_TEMP_WARN;
            if (temp_c >= ALERT_TEMP_CRIT_C)             alerts |= ALERT_BIT_TEMP_CRIT;
            if (ina_ok && v_meas <= ALERT_VOLTAGE_LOW_V) alerts |= ALERT_BIT_UNDERVOLT;
            if (g_state.h2_remaining_pct <= 15.0f)       alerts |= ALERT_BIT_H2_LOW;
            if (!ina_ok)                                 alerts |= ALERT_BIT_INA_ERR;
            g_state.alerts_mask = alerts;

            xSemaphoreGive(g_stateMutex);
        }
    }
}

// ============================================================================
// TÂCHE CORE 1 : ÉMISSION TÉLÉMÉTRIQUE LORA 868 MHZ & LOG SÉRIE (10 HZ)
// ============================================================================
void taskTelemetry10Hz(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(TELEMETRY_TX_PERIOD_MS);
    TelemetryPacket_t txPacket;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        SystemState snap;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            snap = g_state;
            xSemaphoreGive(g_stateMutex);
        }

        // 1. Remplissage de la structure binaire de 20 octets
        txPacket.timestamp_ms  = millis();
        txPacket.voltage_mv    = (uint16_t)constrain(snap.voltage_v * 1000.0f, 0.0f, 65535.0f);
        txPacket.current_ma    = (int16_t)constrain(snap.current_a * 1000.0f, -32768.0f, 32767.0f);
        txPacket.speed_cm_s    = (uint16_t)constrain(snap.speed_kmh * 27.7778f, 0.0f, 65535.0f);
        txPacket.rpm_shaft     = (uint16_t)constrain(snap.rpm_shaft, 0.0f, 65535.0f);
        txPacket.temp_stack_dC = (int16_t)constrain(snap.temp_stack_c * 10.0f, -32768.0f, 32767.0f);
        txPacket.throttle_pct  = snap.throttle_pct;
        txPacket.h2_remaining  = (uint8_t)roundf(snap.h2_remaining_pct);
        txPacket.alerts_mask   = snap.alerts_mask;

        // 2. Calcul du CRC16 sur les 18 premiers octets
        txPacket.crc16 = computeCRC16((const uint8_t*)&txPacket, sizeof(TelemetryPacket_t) - 2);

        // 3. Transmission Radio LoRa 868 MHz (si module SX1262 présent)
        if (lora_initialized) {
            radio.transmit((uint8_t*)&txPacket, sizeof(TelemetryPacket_t));
        }

        // 4. Sortie Série CSV temps réel (compatible Dashboard WebSerial & Debug)
        // Format: CSV;timestamp_ms;U(V);I(A);P(W);E(Wh);Vitesse(km/h);RPM;Temp(C);Gaz(%);H2(%);Alertes;CRC16
        Serial.printf("CSV;%lu;%.2f;%.2f;%.2f;%.3f;%.1f;%.0f;%.1f;%u;%u;0x%02X;0x%04X\n",
                      (unsigned long)txPacket.timestamp_ms,
                      snap.voltage_v,
                      snap.current_a,
                      snap.power_w,
                      snap.energy_wh,
                      snap.speed_kmh,
                      snap.rpm_shaft,
                      snap.temp_stack_c,
                      txPacket.throttle_pct,
                      txPacket.h2_remaining,
                      txPacket.alerts_mask,
                      txPacket.crc16);
    }
}

// ============================================================================
// INITIALISATION GÉNÉRALE (SETUP)
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n=======================================================");
    Serial.println("   H2-VISION — INITIALISATION DU BOITIER EMBARQUE      ");
    Serial.println("=======================================================");

    g_stateMutex = xSemaphoreCreateMutex();

    // 1. Configuration ADC Sonde NTC 10k
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_NTC_ADC, ADC_11db); // Plage complète 0 à 3.3V

    // 2. Configuration Interruptions Capteur Hall & Gâchette PWM
    pinMode(PIN_HALL_ISR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_HALL_ISR), onHallPulseISR, FALLING);

    pinMode(PIN_THROTTLE_PWM, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_THROTTLE_PWM), onThrottlePwmISR, CHANGE);

    // 3. Initialisation Bus I2C (400 kHz Fast Mode) & Capteur INA226
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000UL);
    ina226_initialized = initINA226();
    if (ina226_initialized) {
        Serial.println("[OK] Capteur I2C TI INA226 detecte et configure (Shunt 0.02 Ohm).");
    } else {
        Serial.println("[WARN] Capteur INA226 non detecte sur I2C (0x40) — Mode autonome actif.");
    }

    // 4. Initialisation Bus SPI & Transceiver LoRa Semtech SX1262 (868 MHz)
    loraSPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_NSS);
    lora_initialized = radio.begin(LORA_FREQ_MHZ, LORA_TX_POWER_DBM);
    if (lora_initialized) {
        Serial.println("[OK] Transceiver LoRa SX1262 initialise (868.1 MHz, SF7, BW 500kHz, +14dBm).");
    } else {
        Serial.println("[WARN] Module LoRa SX1262 non detecte sur SPI — Sortie Serie USB uniquement.");
    }

    // 5. Initialisation Wi-Fi Point d'Accès & Serveur Web Embarqué
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS, WIFI_AP_CHANNEL);
    Serial.printf("[OK] Wi-Fi AP actif : SSID '%s' (Mdp: %s) -> IP: %s\n",
                  WIFI_AP_SSID, WIFI_AP_PASS, WiFi.softAPIP().toString().c_str());

    // Route racine : Sert le Dashboard HTML officiel
    webServer.on("/", HTTP_GET, []() {
        webServer.send_P(200, "text/html", INDEX_HTML);
    });

    // Route API JSON : Flux de télémétrie temps réel pour le Dashboard
    webServer.on("/data", HTTP_GET, []() {
        SystemState snap;
        if (xSemaphoreTake(g_stateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            snap = g_state;
            xSemaphoreGive(g_stateMutex);
        }

        char json[256];
        snprintf(json, sizeof(json),
                 "{\"spd\":%.1f,\"rpm\":%.0f,\"V\":%.2f,\"I\":%.2f,\"P\":%.1f,\"T\":%.1f,\"g\":%u,\"wh\":%.2f,\"h2\":%.1f,\"dist\":%.1f,\"rssi\":-45,\"lost\":0}",
                 snap.speed_kmh,
                 snap.rpm_shaft,
                 snap.voltage_v,
                 snap.current_a,
                 snap.power_w,
                 snap.temp_stack_c,
                 snap.throttle_pct,
                 snap.energy_wh,
                 snap.h2_consumed_g * 1000.0f, // conversion g -> mg pour le JS
                 snap.distance_m);

        webServer.sendHeader("Access-Control-Allow-Origin", "*");
        webServer.send(200, "application/json", json);
    });

    webServer.begin();
    Serial.println("[OK] Serveur Web demarre sur http://192.168.4.1");

    // 6. Lancement des tâches temps réel sur les 2 cœurs de l'ESP32
    xTaskCreatePinnedToCore(
        taskAcquisition50Hz,   // Fonction de la tâche
        "Acq50Hz_Core0",       // Nom debug
        4096,                  // Taille pile (octets)
        NULL,                  // Paramètre
        3,                     // Priorité haute (Temps réel)
        NULL,                  // Handle
        0                      // Épinglée sur le CORE 0
    );

    xTaskCreatePinnedToCore(
        taskTelemetry10Hz,     // Fonction de la tâche
        "LoraTx10Hz_Core1",    // Nom debug
        4096,                  // Taille pile (octets)
        NULL,                  // Paramètre
        1,                     // Priorité normale
        NULL,                  // Handle
        1                      // Épinglée sur le CORE 1
    );

    Serial.println("[OK] Ordonnanceur FreeRTOS Dual-Core demarre !");
    Serial.println("Format: CSV;timestamp_ms;U(V);I(A);P(W);E(Wh);Vitesse(km/h);RPM;Temp(C);Gaz(%);H2(%);Alertes;CRC16");
}

void loop() {
    // Gestionnaire des requêtes HTTP du Dashboard Web sur le Cœur 1
    webServer.handleClient();
    delay(2);
}
