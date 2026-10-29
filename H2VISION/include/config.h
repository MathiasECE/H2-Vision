#ifndef H2_VISION_CONFIG_H
#define H2_VISION_CONFIG_H

#include <Arduino.h>

// ============================================================================
// 1. AFFECTATION DES BROCHES (PINOUT COMPATIBLE ESP32-S3 & ESP32 CLASSIQUE)
// ============================================================================
#if CONFIG_IDF_TARGET_ESP32S3
    // Brochage pour ESP32-S3 DevKitC-1 (conforme au schéma KiCad H2-Vision)
    #define PIN_NTC_ADC         1   // ADC1_CH0 : Sonde température NTC 10k
    #define PIN_HALL_ISR        4   // Interruption vitesse (Capteur Hall A3144)
    #define PIN_THROTTLE_PWM    5   // Capture impulsion PWM gâchette (Câble Y)
    #define PIN_I2C_SDA         8   // Bus I2C Données (INA226)
    #define PIN_I2C_SCL         9   // Bus I2C Horloge (INA226)
    #define PIN_LORA_NSS        10  // SPI Chip Select SX1262
    #define PIN_LORA_MOSI       11  // SPI MOSI SX1262
    #define PIN_LORA_SCK        12  // SPI Clock SX1262
    #define PIN_LORA_MISO       13  // SPI MISO SX1262
    #define PIN_LORA_BUSY       14  // Busy Status SX1262
    #define PIN_LORA_DIO1       21  // Interruption IRQ SX1262
    #define PIN_LORA_RST        47  // Reset matériel SX1262
#else
    // Brochage pour ESP32 Classique (NodeMCU-32S / Wokwi)
    #define PIN_NTC_ADC         32  // ADC1_CH4 : Sonde température NTC 10k
    #define PIN_HALL_ISR        4   // Interruption vitesse (Capteur Hall A3144)
    #define PIN_THROTTLE_PWM    15  // Capture impulsion PWM gâchette (Câble Y)
    #define PIN_I2C_SDA         21  // Bus I2C Données (INA226)
    #define PIN_I2C_SCL         22  // Bus I2C Horloge (INA226)
    #define PIN_LORA_NSS        5   // SPI Chip Select SX1262
    #define PIN_LORA_MOSI       23  // SPI MOSI SX1262
    #define PIN_LORA_SCK        18  // SPI Clock SX1262
    #define PIN_LORA_MISO       19  // SPI MISO SX1262
    #define PIN_LORA_BUSY       26  // Busy Status SX1262
    #define PIN_LORA_DIO1       25  // Interruption IRQ SX1262
    #define PIN_LORA_RST        27  // Reset matériel SX1262
#endif

// ============================================================================
// 2. PARAMÈTRES PHYSIQUES DU VÉHICULE & CAPTEURS
// ============================================================================

// --- Capteur Électrique TI INA226 ---
#define INA226_I2C_ADDR         0x40        // Adresse I2C par défaut (A0=GND, A1=GND)
#define INA226_R_SHUNT_OHM      0.020f      // Résistance de shunt : 20 mOhm (0.02 Ohm)

// --- Loi de Faraday & Jauge Hydrogène (Pile Horizon H-30 + Hydrostik PRO) ---
#define FARADAY_CONSTANT        96485.33f   // Constante de Faraday (C/mol)
#define H2_MOLAR_MASS           2.016f      // Masse molaire du dihydrogène (g/mol)
#define FUEL_CELL_CELLS_COUNT   1           // Facteur stœchiométrique équivalent par électron
#define FARADAY_EFFICIENCY      0.98f       // Rendement faradique de la pile (purges incluses)
#define HYDROSTIK_CAPACITY_G    0.90f       // Masse utile d'H2 dans une cartouche pleine (g)

// --- Capteur Hall A3144 & Cinématique Châssis 1/10 4WD ---
#define WHEEL_DIAMETER_M        0.065f      // Diamètre extérieur pneu piste : 65 mm
#define TRANSMISSION_RATIO      2.50f       // Rapport couronne/pignon différentiel (arbre -> roue)
#define MAGNETS_PER_REV         1           // Nombre d'aimants collés sur l'arbre central
#define HALL_DEBOUNCE_US        1000UL      // Blanking time anti-rebond : 1000 µs (max 60 000 RPM)
#define HALL_TIMEOUT_US         500000UL    // Délai sans impulsion avant vitesse = 0 (0.5 s)

// --- Thermistance CTN 10k B3950 (Pont diviseur 3.3V) ---
#define NTC_R_SERIES_OHM        10000.0f    // Résistance fixe du pont diviseur (10 kOhm)
#define NTC_R_NOMINAL_OHM       10000.0f    // Résistance nominale CTN à 25°C (10 kOhm)
#define NTC_TEMP_NOMINAL_C      25.0f       // Température nominale (25°C)
#define NTC_BETA_COEFF          3950.0f     // Coefficient Beta de la thermistance B3950
#define ADC_VREF_MV             3300.0f     // Tension de référence ADC ESP32 (3.3V)

// --- Cordon Servo en Y (Signal PWM Gâchette CH2) ---
#define PWM_MIN_PULSE_US        1000UL      // Impulsion repos / frein (1000 µs)
#define PWM_NEUTRAL_PULSE_US    1500UL      // Impulsion point mort (1500 µs)
#define PWM_MAX_PULSE_US        2000UL      // Impulsion plein gaz (2000 µs)

// --- Paramètres Wi-Fi Point d'Accès (Dashboard Web Embarqué) ---
#define WIFI_AP_SSID            "H2-VISION"
#define WIFI_AP_PASS            "h2gp2026"
#define WIFI_AP_CHANNEL         6

// --- Paramètres Radio LoRa Semtech SX1262 (Bande ISM 868 MHz) ---
#define LORA_FREQ_MHZ           868.1f      // Fréquence centrale 868.1 MHz
#define LORA_BW_KHZ             500.0f      // Bande passante 500 kHz (Time-on-Air ultra-court)
#define LORA_SF                 7           // Spreading Factor SF7
#define LORA_CR                 5           // Coding Rate 4/5
#define LORA_TX_POWER_DBM       14          // Puissance d'émission +14 dBm (25 mW, légal Europe)

// --- Seuils d'Alertes de Sécurité ---
#define ALERT_TEMP_WARN_C       50.0f       // Seuil d'alerte orange température pile (°C)
#define ALERT_TEMP_CRIT_C       55.0f       // Seuil critique assèchement membrane Nafion (°C)
#define ALERT_VOLTAGE_LOW_V     5.5f        // Seuil de sous-tension critique pile (V)

// --- Fréquences d'Exécution Temps Réel (FreeRTOS) ---
#define ACQUISITION_PERIOD_MS   20          // Core 0 : Échantillonnage capteurs à 50 Hz (20 ms)
#define TELEMETRY_TX_PERIOD_MS  100         // Core 1 : Émission LoRa & Série à 10 Hz (100 ms)

#endif // H2_VISION_CONFIG_H
