#ifndef H2_VISION_SX1262_DRIVER_H
#define H2_VISION_SX1262_DRIVER_H

#include <Arduino.h>
#include <SPI.h>
#include "config.h"

/**
 * Pilote SPI natif pour le transceiver LoRa Semtech SX1262 (Waveshare Core1262-868M).
 * Conforme à la datasheet officielle Semtech DS_SX1261-2_V2.1 (100% autonome, sans librairie externe).
 */
class SX1262Driver {
private:
    SPIClass &_spi;
    int _pinNss, _pinDio1, _pinRst, _pinBusy;

    void waitBusy(uint32_t timeout_ms = 50) {
        uint32_t start = millis();
        while (digitalRead(_pinBusy) == HIGH) {
            if ((millis() - start) > timeout_ms) break;
            delayMicroseconds(10);
        }
    }

    void writeCommand(uint8_t opcode, const uint8_t *params, uint8_t len) {
        waitBusy();
        digitalWrite(_pinNss, LOW);
        _spi.transfer(opcode);
        for (uint8_t i = 0; i < len; i++) {
            _spi.transfer(params[i]);
        }
        digitalWrite(_pinNss, HIGH);
        if (opcode != 0x84) { // Sauf SetSleep
            waitBusy();
        }
    }

public:
    SX1262Driver(SPIClass &spi, int nss, int dio1, int rst, int busy)
        : _spi(spi), _pinNss(nss), _pinDio1(dio1), _pinRst(rst), _pinBusy(busy) {}

    bool begin(float freq_mhz = LORA_FREQ_MHZ, int8_t tx_power_dbm = LORA_TX_POWER_DBM) {
        pinMode(_pinNss, OUTPUT);
        digitalWrite(_pinNss, HIGH);
        pinMode(_pinRst, OUTPUT);
        pinMode(_pinBusy, INPUT);
        pinMode(_pinDio1, INPUT);

        // 1. Reset matériel du module SX1262
        digitalWrite(_pinRst, LOW);
        delay(2);
        digitalWrite(_pinRst, HIGH);
        delay(10);

        // Vérifie que la broche BUSY redescend à l'état bas (module présent)
        if (digitalRead(_pinBusy) == HIGH) {
            delay(20);
            if (digitalRead(_pinBusy) == HIGH) return false;
        }

        // 2. Lecture registre version/status (GetStatus = 0xC0) pour vérifier la présence SPI
        digitalWrite(_pinNss, LOW);
        uint8_t status = _spi.transfer(0xC0);
        _spi.transfer(0x00);
        digitalWrite(_pinNss, HIGH);
        if (status == 0x00 || status == 0xFF) {
            return false; // Pas de module sur le bus SPI
        }

        // 3. SetStandby (STDBY_RC = 0x00) -> Opcode 0x80
        uint8_t stby = 0x00;
        writeCommand(0x80, &stby, 1);

        // 4. SetPacketType (PACKET_TYPE_LORA = 0x01) -> Opcode 0x8A
        uint8_t pktType = 0x01;
        writeCommand(0x8A, &pktType, 1);

        // 5. SetRfFrequency -> Opcode 0x86 (Freq_Reg = Freq_Hz * 2^25 / 32e6)
        uint32_t frf = (uint32_t)((freq_mhz * 1000000.0) * (33554432.0 / 32000000.0));
        uint8_t freqParams[4] = {
            (uint8_t)((frf >> 24) & 0xFF),
            (uint8_t)((frf >> 16) & 0xFF),
            (uint8_t)((frf >> 8) & 0xFF),
            (uint8_t)(frf & 0xFF)
        };
        writeCommand(0x86, freqParams, 4);

        // 6. SetPaConfig (Optimal pour SX1262 jusqu'à +22 dBm) -> Opcode 0x95
        uint8_t paParams[4] = { 0x04, 0x07, 0x00, 0x01 };
        writeCommand(0x95, paParams, 4);

        // 7. SetTxParams (Puissance +14 dBm, RampTime 200 µs = 0x04) -> Opcode 0x8E
        uint8_t txParams[2] = { (uint8_t)tx_power_dbm, 0x04 };
        writeCommand(0x8E, txParams, 2);

        // 8. SetBufferBaseAddress (TX=0x00, RX=0x80) -> Opcode 0x8F
        uint8_t bufParams[2] = { 0x00, 0x80 };
        writeCommand(0x8F, bufParams, 2);

        // 9. SetModulationParams (SF7=0x07, BW_500kHz=0x06, CR_4_5=0x01, LowDataRateOpt=0x00) -> Opcode 0x8B
        uint8_t modParams[4] = { 0x07, 0x06, 0x01, 0x00 };
        writeCommand(0x8B, modParams, 4);

        return true;
    }

    bool transmit(const uint8_t *data, uint8_t len) {
        // 1. SetPacketParams (Preamble=8, ExplicitHeader=0x00, PayloadLen=len, CRC_ON=0x01, StdIQ=0x00) -> Opcode 0x8C
        uint8_t pktParams[6] = { 0x00, 0x08, 0x00, len, 0x01, 0x00 };
        writeCommand(0x8C, pktParams, 6);

        // 2. WriteBuffer (Offset 0x00 + Payload) -> Opcode 0x0E
        waitBusy();
        digitalWrite(_pinNss, LOW);
        _spi.transfer(0x0E);
        _spi.transfer(0x00); // Offset TX
        for (uint8_t i = 0; i < len; i++) {
            _spi.transfer(data[i]);
        }
        digitalWrite(_pinNss, HIGH);
        waitBusy();

        // 3. SetTx (Timeout = 500 ms -> 0x007D00 pas de 15.625 µs) -> Opcode 0x83
        uint8_t txTimeout[3] = { 0x00, 0x7D, 0x00 };
        writeCommand(0x83, txTimeout, 3);

        return true;
    }
};

#endif // H2_VISION_SX1262_DRIVER_H
