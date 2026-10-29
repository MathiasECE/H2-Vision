#ifndef H2_VISION_TELEMETRY_PACKET_H
#define H2_VISION_TELEMETRY_PACKET_H

#include <Arduino.h>

// Masques de bits pour le champ alerts_mask
#define ALERT_BIT_TEMP_WARN     (1 << 0)    // Bit 0 : Température >= 50°C
#define ALERT_BIT_TEMP_CRIT     (1 << 1)    // Bit 1 : Température >= 55°C (Danger membrane)
#define ALERT_BIT_UNDERVOLT     (1 << 2)    // Bit 2 : Sous-tension pile <= 5.5V
#define ALERT_BIT_H2_LOW        (1 << 3)    // Bit 3 : Réserve Hydrogène < 15%
#define ALERT_BIT_INA_ERR       (1 << 4)    // Bit 4 : Défaut lecture I2C INA226

/**
 * Structure binaire compactée de 20 octets envoyée par radio LoRa 868 MHz.
 * L'attribut packed garantit l'absence d'octets de bourrage (padding) mémoire.
 */
typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;  // [0..3]   4 octets : Temps écoulé depuis le boot (ms)
    uint16_t voltage_mv;    // [4..5]   2 octets : Tension pile H2 (0 à 36000 mV, rés. 1 mV)
    int16_t  current_ma;    // [6..7]   2 octets : Courant débité (-20000 à +20000 mA)
    uint16_t speed_cm_s;    // [8..9]   2 octets : Vitesse linéaire (cm/s -> 0.036 km/h)
    uint16_t rpm_shaft;     // [10..11] 2 octets : Vitesse rotation arbre central (tr/min)
    int16_t  temp_stack_dC; // [12..13] 2 octets : Température pile en dixièmes de °C (ex: 485 = 48.5°C)
    uint8_t  throttle_pct;  // [14]     1 octet  : Consigne gâchette pilote (0 à 100 %)
    uint8_t  h2_remaining;  // [15]     1 octet  : Jauge hydrogène restante (0 à 100 %)
    uint16_t alerts_mask;   // [16..17] 2 octets : Masque d'alertes système
    uint16_t crc16;         // [18..19] 2 octets : Checksum d'intégrité CRC16-CCITT
} TelemetryPacket_t;

// Vérification statique à la compilation que la trame fait exactement 20 octets
static_assert(sizeof(TelemetryPacket_t) == 20, "Erreur: TelemetryPacket_t doit faire exactement 20 octets !");

/**
 * Calcul de la somme de contrôle CRC16-CCITT (Polynôme 0x1021, Init 0xFFFF)
 */
inline uint16_t computeCRC16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

#endif // H2_VISION_TELEMETRY_PACKET_H
