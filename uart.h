#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * @brief Abre a porta UART para comunicação RS485-MODBUS
 * @param device Caminho do dispositivo (ex: "/dev/ttyUSB0")
 * @return File descriptor da UART ou -1 em caso de erro
 */
int open_uart(const char *device);

/**
 * @brief Envia dados pela UART
 * @param fd File descriptor da UART
 * @param buffer Buffer com os dados a enviar
 * @param len Tamanho dos dados em bytes
 */
void send_uart(int fd, const uint8_t *buffer, int len);

/**
 * @brief Recebe dados pela UART
 * @param fd File descriptor da UART
 * @param buffer Buffer para armazenar os dados recebidos
 * @param max_len Tamanho máximo do buffer
 * @return Número de bytes lidos
 */
int receive_uart(int fd, uint8_t *buffer, int max_len);

/**
 * @brief Fecha a porta UART
 * @param fd File descriptor da UART
 */
void close_uart(int fd);

#endif
