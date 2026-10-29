/*
 * H2-VISION : Firmware de Télémétrie Embarquée (Simulation Wokwi)
 * Compétition : Horizon Hydrogen Grand Prix (H2GP Stock)
 * Équipe : ECE Paris / Horizon Educational
 *
 * Ce code simule :
 * 1. Capteur Hall (GPIO 4) : Interruptions temps réel, RPM & Vitesse (km/h)
 * 2. Sonde NTC 10k (GPIO 32) : Conversion analogique en °C (Steinhart-Hart)
 * 3. Mesure INA226 (GPIO 34) : Courant (0-15A) et Tension dynamique avec sag
 * 4. Gâchette Pilote (GPIO 35) : Position PWM (0-100%)
 * 5. Algorithme de Faraday : Intégration continue de la jauge H2 restante
 * 6. Écran OLED I2C (SSD1306) : Dashboard local en direct
 * 7. Sérialisation de la trame binaire de 20 octets (avec CRC16)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- PINOUT ---
const int PIN_HALL = 4;        // Entrée Interruption Capteur Hall
const int PIN_CURRENT = 34;    // Simulation Courant INA226 (ADC)
const int PIN_THROTTLE = 35;   // Simulation Gâchette Pilote (ADC)
const int PIN_NTC = 32;        // Simulation Sonde Température (ADC)

// --- CONSTANTES PHYSIQUES VOITURE & PILE ---
const float WHEEL_DIAMETER_M = 0.065;      // Diamètre roue : 65 mm (1/10 piste)
const float GEAR_RATIO = 2.5;              // Rapport transmission arbre / roues
const float FARADAY_CONST = 96485.33;      // Constante de Faraday (C/mol)
const float MOLAR_MASS_H2 = 2.016;         // Masse molaire H2 (g/mol)
const float INITIAL_H2_GRAMS = 0.90;       // Capacité utile cartouche Hydrostik PRO (g)

// --- VARIABLES GLOBALES INTERRUPTIONS & DSP ---
volatile unsigned long last_hall_us = 0;
volatile unsigned long period_us = 0;
volatile unsigned long pulse_count = 0;

float speed_kmh = 0.0;
float rpm_shaft = 0.0;
float voltage_v = 7.4;
float current_a = 0.0;
float power_w = 0.0;
float temp_c = 25.0;
float throttle_pct = 0.0;
float h2_remaining_grams = INITIAL_H2_GRAMS;
float h2_pct = 100.0;
float energy_wh = 0.0;

unsigned long last_loop_ms = 0;
unsigned long last_display_ms = 0;

// --- STRUCTURE DE LA TRAME BINAIRE TELEMETRIE (20 OCTETS) ---
typedef struct __attribute__((packed)) {
  uint32_t timestamp_ms; // 4 octets
  uint16_t voltage_mv;   // 2 octets
  int16_t  current_ma;   // 2 octets
  uint16_t speed_cm_s;   // 2 octets
  uint16_t rpm_shaft;    // 2 octets
  int16_t  temp_c_x10;   // 2 octets (résolution 0.1°C)
  uint8_t  throttle_pct; // 1 octet
  uint8_t  h2_remaining; // 1 octet (0 à 100%)
  uint16_t alerts_mask;  // 2 octets (Bit 0: Surchauffe, Bit 1: Sous-tension)
  uint16_t crc16;        // 2 octets (Contrôle intégrité)
} TelemetryPacket_t;

TelemetryPacket_t packet;

// --- ROUTINE D'INTERRUPTION CAPTEUR HALL (ISR) ---
void IRAM_ATTR onHallPulse() {
  unsigned long now = micros();
  unsigned long dt = now - last_hall_us;
  if (dt > 1000) { // Filtre anti-rebond matériel 1 ms (> 60 000 tr/min max)
    period_us = dt;
    last_hall_us = now;
    pulse_count++;
  }
}

// --- CALCUL DU CHECKSUM CRC16-CCITT ---
uint16_t computeCRC16(const uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else crc <<= 1;
    }
  }
  return crc;
}

void setup() {
  Serial.begin(115200);

  // Configuration des broches
  pinMode(PIN_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), onHallPulse, FALLING);

  // Initialisation Écran OLED I2C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Échec initialisation SSD1306"));
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(15, 20);
  display.println("H2-VISION TELEMETRY");
  display.setCursor(30, 35);
  display.println("ECE Paris 2026");
  display.display();
  delay(1500);

  Serial.println("=================================================");
  Serial.println("   H2-VISION — SYSTEME DE TELEMETRIE DEMARRE    ");
  Serial.println("=================================================");
  Serial.println("Temps(s);Tension(V);Courant(A);Puiss(W);Vitesse(km/h);Temp(C);H2(%);Gaz(%)");

  last_loop_ms = millis();
}

void loop() {
  unsigned long now_ms = millis();
  float dt_sec = (now_ms - last_loop_ms) / 1000.0;
  last_loop_ms = now_ms;

  // 1. LECTURE & CONVERSION VITESSE (CAPTEUR HALL)
  if (micros() - last_hall_us > 400000) { // Si arrêté depuis > 0.4s
    speed_kmh = 0.0;
    rpm_shaft = 0.0;
  } else if (period_us > 0) {
    rpm_shaft = 60000000.0 / period_us;
    float wheel_rpm = rpm_shaft / GEAR_RATIO;
    speed_kmh = (wheel_rpm * 3.14159 * WHEEL_DIAMETER_M * 60.0) / 1000.0;
  }

  // 2. LECTURE GÂCHETTE PILOTE (ADC 0-100%)
  int raw_throttle = analogRead(PIN_THROTTLE);
  throttle_pct = map(raw_throttle, 0, 4095, 0, 100);

  // 3. LECTURE COURANT & TENSION (SIMULATION INA226)
  int raw_current = analogRead(PIN_CURRENT);
  current_a = (raw_current / 4095.0) * 15.0; // 0 à 15 A
  // Modèle de chute de tension de la pile sous charge (Sag)
  voltage_v = 8.4 - (current_a * 0.12); 
  if (voltage_v < 6.0) voltage_v = 6.0;
  power_w = voltage_v * current_a;
  energy_wh += (power_w * dt_sec) / 3600.0;

  // 4. ALGORITHME DE FARADAY (CALCUL JAUGE HYDROGÈNE)
  // dn = (I * dt) / (2 * F) en moles
  float dn_mol = (current_a * dt_sec) / (2.0 * FARADAY_CONST);
  float dm_grams = dn_mol * MOLAR_MASS_H2;
  h2_remaining_grams -= dm_grams;
  if (h2_remaining_grams < 0) h2_remaining_grams = 0;
  h2_pct = (h2_remaining_grams / INITIAL_H2_GRAMS) * 100.0;

  // 5. LECTURE TEMPÉRATURE NTC (STEINHART-HART)
  int raw_ntc = analogRead(PIN_NTC);
  float v_ntc = (raw_ntc / 4095.0) * 3.3;
  if (v_ntc > 0.1 && v_ntc < 3.2) {
    float r_ntc = 10000.0 * v_ntc / (3.3 - v_ntc);
    float steinhart;
    steinhart = r_ntc / 10000.0;          // (R/Ro)
    steinhart = log(steinhart);            // ln(R/Ro)
    steinhart /= 3950.0;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (25.0 + 273.15);    // + (1/To)
    steinhart = 1.0 / steinhart;           // Inverser
    temp_c = steinhart - 273.15;           // Kelvin vers Celsius
  }

  // 6. ENCAPSULATION DE LA TRAME BINAIRE (20 OCTETS)
  packet.timestamp_ms = now_ms;
  packet.voltage_mv = (uint16_t)(voltage_v * 1000.0);
  packet.current_ma = (int16_t)(current_a * 1000.0);
  packet.speed_cm_s = (uint16_t)(speed_kmh * 27.7778); // km/h -> cm/s
  packet.rpm_shaft = (uint16_t)rpm_shaft;
  packet.temp_c_x10 = (int16_t)(temp_c * 10.0);
  packet.throttle_pct = (uint8_t)throttle_pct;
  packet.h2_remaining = (uint8_t)h2_pct;
  
  // Masque d'alertes de sécurité
  packet.alerts_mask = 0;
  if (temp_c >= 50.0) packet.alerts_mask |= (1 << 0); // Surchauffe pile
  if (voltage_v <= 6.2) packet.alerts_mask |= (1 << 1); // Sous-tension

  // Calcul du CRC sur les 18 premiers octets
  packet.crc16 = computeCRC16((uint8_t*)&packet, sizeof(TelemetryPacket_t) - 2);

  // 7. RAFRAÎCHISSEMENT AFFICHAGE OLED & CSV SÉRIE (TOUS LES 200 MS)
  if (now_ms - last_display_ms >= 200) {
    last_display_ms = now_ms;

    // Affichage OLED
    display.clearDisplay();
    display.setTextSize(1);
    
    // Ligne 1 : Vitesse & Gaz
    display.setCursor(0, 0);
    display.printf("VIT: %4.1f km/h  G:%2d%%", speed_kmh, (int)throttle_pct);

    // Ligne 2 : Puissance & Tension
    display.setCursor(0, 14);
    display.printf("PWR: %4.1f W (%3.1fV)", power_w, voltage_v);

    // Ligne 3 : Courant & Température
    display.setCursor(0, 28);
    display.printf("AMP: %4.1f A  T:%3.0fC", current_a, temp_c);

    // Ligne 4 : Jauge Hydrogène Faraday
    display.setCursor(0, 44);
    display.printf("H2 RESTANT: %3d %%", (int)h2_pct);
    display.drawRect(0, 56, 128, 8, SSD1306_WHITE);
    int bar_width = map((int)h2_pct, 0, 100, 0, 126);
    display.fillRect(1, 57, bar_width, 6, SSD1306_WHITE);

    display.display();

    // Export CSV sur le port Série
    Serial.printf("%.1f;%.2f;%.2f;%.1f;%.1f;%.1f;%d;%d\n",
      now_ms / 1000.0, voltage_v, current_a, power_w, speed_kmh, temp_c, (int)h2_pct, (int)throttle_pct);
  }

  delay(20); // Boucle à 50 Hz
}
