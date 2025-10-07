#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

/**
 * @brief Calcula o CRC16 MODBUS
 * @param data Ponteiro para os dados
 * @param len Tamanho dos dados em bytes
 * @return CRC16 calculado
 */
uint16_t crc16_modbus(const uint8_t *data, int len);

#endif
