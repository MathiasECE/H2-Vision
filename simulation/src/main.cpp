/**
 * ============================================================================
 * PROJET H2-VISION — SIMULATEUR MATERIEL HARDWARE-IN-THE-LOOP (ESP32 N°2)
 * Rôle : Générateur de signaux physiques pour tester l'ESP32 N°1 et l'INA226
 * ============================================================================
 * 
 * Ce code transforme un second ESP32 en banc d'essai complet :
 *  1. Générateur de Courant Moteur (PWM haute fréquence) sur GPIO 18
 *     -> Se branche sur IN+ de l'INA226 (avec résistance 150-220 Ohm vers GND)
 *  2. Générateur de Gâchette RC Pilote (PWM 50 Hz, 1000 à 2000 µs) sur GPIO 19
 *     -> Se branche sur GPIO 5 de l'ESP32 N°1 (entrée Câble Y)
 *  3. Simulateur d'Impulsions Vitesse (Capteur Hall) sur GPIO 23
 *     -> Se branche sur GPIO 4 de l'ESP32 N°1 (entrée Hall A3144)
 */

#include <Arduino.h>

// ============================================================================
// CONFIGURATION DES BROCHES DE SIMULATION (ESP32 N°2)
// ============================================================================
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
  // Configuration pour ESP32-S3 (Evite GPIO 19/20 reserves pour l'USB natif)
  #define PIN_CURRENT_OUT     18  // Sortie courant module -> vers IN+ de l'INA226
  #define PIN_THROTTLE_OUT    7   // Sortie PWM gachette -> vers GPIO 5 de l'ESP32 N°1
  #define PIN_HALL_OUT        21  // Sortie impulsions Hall -> vers GPIO 4 de l'ESP32 N°1
  #define BOARD_NAME          "ESP32-S3"
#else
  // Configuration pour ESP32 classique (NodeMCU-32S / ESP32-WROOM-32)
  #define PIN_CURRENT_OUT     18  // Sortie courant module -> vers IN+ de l'INA226
  #define PIN_THROTTLE_OUT    19  // Sortie PWM gachette -> vers GPIO 5 de l'ESP32 N°1
  #define PIN_HALL_OUT        23  // Sortie impulsions Hall -> vers GPIO 4 de l'ESP32 N°1
  #define BOARD_NAME          "NodeMCU-32S"
#endif

// Canaux LEDC PWM
#define CH_CURRENT          0   // Canal PWM pour le courant
#define CH_THROTTLE         1   // Canal PWM pour la gâchette RC (50 Hz)

// ============================================================================
// INITIALISATION
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=======================================================");
    Serial.println("   H2-VISION — SIMULATEUR PHYSIQUE DE COURSE (ESP32 #2)");
    Serial.println("=======================================================");

    // 1. Configuration du Générateur de Courant (5 kHz, 8 bits : 0 à 255)
    ledcSetup(CH_CURRENT, 5000, 8);
    ledcAttachPin(PIN_CURRENT_OUT, CH_CURRENT);

    // 2. Configuration du Générateur de Gâchette RC (50 Hz, 16 bits : 20 ms)
    ledcSetup(CH_THROTTLE, 50, 16);
    ledcAttachPin(PIN_THROTTLE_OUT, CH_THROTTLE);

    // 3. Configuration de la sortie impulsion vitesse (Tout-ou-rien)
    pinMode(PIN_HALL_OUT, OUTPUT);
    digitalWrite(PIN_HALL_OUT, HIGH); // Repos à l'état haut (comme le capteur Hall avec pull-up)

    Serial.printf("[OK] Simulateur pret (%s) :\n", BOARD_NAME);
    Serial.printf("  - GPIO %d : Sortie Courant vers IN+ de l'INA226 (avec R = 150-220 Ohm)\n", PIN_CURRENT_OUT);
    Serial.printf("  - GPIO %d : Sortie Gachette PWM vers GPIO 5 de l'ESP32 #1\n", PIN_THROTTLE_OUT);
    Serial.printf("  - GPIO %d : Sortie Vitesse Hall vers GPIO 4 de l'ESP32 #1\n", PIN_HALL_OUT);
    Serial.println("  - GND     : Relier obligatoirement au GND de l'ESP32 #1 !\n");
}

// Fonction utilitaire : applique une impulsion PWM RC standard (1000 à 2000 µs)
void setRcThrottlePercent(float pct) {
    pct = constrain(pct, 0.0f, 100.0f);
    uint32_t pulse_us = 1000 + (uint32_t)(pct * 10.0f); // 1000 µs (0%) à 2000 µs (100%)
    // Résolution 16 bits sur période 20 000 µs (50 Hz) : duty = (pulse_us * 65535) / 20000
    uint32_t duty = (pulse_us * 65535UL) / 20000UL;
    ledcWrite(CH_THROTTLE, duty);
}

// Fonction utilitaire : applique le courant moteur simulé (0 à 255)
void setMotorCurrentDuty(uint8_t duty) {
    ledcWrite(CH_CURRENT, duty);
}

// ============================================================================
// BOUCLE PRINCIPALE : CYCLE DE COURSE DYNAMIQUE (TOUR DE CIRCUIT)
// ============================================================================
void loop() {
    uint32_t now = millis();
    float t_s = now * 0.001f;

    // Cycle de course de 16 secondes par tour :
    float lap_phase = fmodf(t_s, 16.0f);
    float throttle_pct = 0.0f;
    uint8_t current_duty = 0;
    const char* phase_name = "";

    if (lap_phase < 4.0f) {
        // Phase 1 : Sortie de virage, réaccélération progressive
        phase_name = "Relance en sortie de virage";
        throttle_pct = 20.0f + (lap_phase / 4.0f) * 65.0f; // 20% -> 85%
        current_duty = (uint8_t)map((long)throttle_pct, 0, 100, 10, 200);
    } 
    else if (lap_phase < 8.0f) {
        // Phase 2 : Pleine ligne droite (100% gaz, courant max)
        phase_name = "Plein gaz en ligne droite !";
        throttle_pct = 98.0f + 2.0f * sinf(t_s * 5.0f);
        current_duty = 245 + (uint8_t)(10.0f * sinf(t_s * 5.0f));
    } 
    else if (lap_phase < 10.5f) {
        // Phase 3 : Gros freinage (0% gaz, courant de repos)
        phase_name = "Zone de gros freinage";
        throttle_pct = 0.0f;
        current_duty = 0;
    } 
    else if (lap_phase < 13.5f) {
        // Phase 4 : Enchaînement sinueux technique
        phase_name = "Chicane technique mi-regime";
        throttle_pct = 45.0f + 15.0f * sinf(t_s * 2.0f);
        current_duty = (uint8_t)map((long)throttle_pct, 0, 100, 10, 150);
    } 
    else {
        // Phase 5 : Courbe rapide
        phase_name = "Courbe rapide";
        throttle_pct = 75.0f;
        current_duty = 180;
    }

    // Application des signaux physiques
    setRcThrottlePercent(throttle_pct);
    setMotorCurrentDuty(current_duty);

    // Simulation d'une impulsion Hall (vitesse) proportionnelle au régime moteur
    if (throttle_pct > 5.0f) {
        // Fréquence des impulsions selon la vitesse (impulsion basse de 2 ms)
        digitalWrite(PIN_HALL_OUT, LOW);
        delayMicroseconds(1500);
        digitalWrite(PIN_HALL_OUT, HIGH);
    }

    // Affichage moniteur série toutes les 500 ms
    static uint32_t last_print = 0;
    if (now - last_print >= 500) {
        last_print = now;
        Serial.printf("[SIMU #2] %-28s | Gaz: %3.0f %% | Courant PWM: %3u/255\n",
                      phase_name, throttle_pct, current_duty);
    }

    delay(20); // Boucle à 50 Hz
}
