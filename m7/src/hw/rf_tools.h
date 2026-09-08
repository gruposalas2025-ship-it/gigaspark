/*
 * Gigaspark OS - Driver de Herramientas RF
 * CC1101 + PN532 NFC + IR TX/RX
 *
 * CC1101: D23=CS, D15=GDO0, D16=GDO2, SPI compartido con SD
 * PN532:  D20=SDA, D21=SCL (I2C1)
 * IR TX:  D17 (via transistor NPN)
 * IR RX:  D18 (TSOP38238)
 */

#ifndef GIGASPARK_RF_TOOLS_H
#define GIGASPARK_RF_TOOLS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RF_MODULE_CC1101  0
#define RF_MODULE_PN532   1
#define RF_MODULE_IR      2

#define RF_FREQ_433MHZ   433

int rf_init_all(void);
int rf_cc1101_init(uint16_t freq_mhz);
int rf_send_433(const uint8_t *data, size_t len);
int rf_receive_433(uint8_t *buf, size_t max_len);
int rf_nfc_init(void);
int rf_nfc_read_card(uint8_t *uid, uint8_t *uid_len);
int rf_nfc_write_block(uint8_t block_num, const uint8_t *data);
int rf_ir_init(void);
int rf_ir_send_nec(uint8_t address, uint8_t command);
int rf_ir_receive(uint8_t *address, uint8_t *command);
bool rf_is_ready(uint8_t module);

#endif
