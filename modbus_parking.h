#ifndef MODBUS_PARKING_H
#define MODBUS_PARKING_H

#include <stdint.h>

// Endereços dos dispositivos MODBUS
#define CAMERA_ENTRADA_ADDR  0x11
#define CAMERA_SAIDA_ADDR    0x12
#define PLACAR_VAGAS_ADDR    0x20

// Códigos de função MODBUS
#define MODBUS_READ_HOLDING_REGS   0x03
#define MODBUS_WRITE_MULTIPLE_REGS 0x10

// Offsets dos registradores - Câmeras LPR
#define LPR_STATUS_OFFSET      0
#define LPR_TRIGGER_OFFSET     1
#define LPR_PLACA_OFFSET       2
#define LPR_CONFIANCA_OFFSET   6
#define LPR_ERRO_OFFSET        7

// Status da câmera LPR
#define LPR_STATUS_PRONTO      0
#define LPR_STATUS_PROCESSANDO 1
#define LPR_STATUS_OK          2
#define LPR_STATUS_ERRO        3

// Offsets dos registradores - Placar de Vagas
#define PLACAR_VAGAS_TERREO_PNE      0
#define PLACAR_VAGAS_TERREO_IDOSO    1
#define PLACAR_VAGAS_TERREO_COMUNS   2
#define PLACAR_VAGAS_1ANDAR_PNE      3
#define PLACAR_VAGAS_1ANDAR_IDOSO    4
#define PLACAR_VAGAS_1ANDAR_COMUNS   5
#define PLACAR_VAGAS_2ANDAR_PNE      6
#define PLACAR_VAGAS_2ANDAR_IDOSO    7
#define PLACAR_VAGAS_2ANDAR_COMUNS   8
#define PLACAR_CARROS_TERREO         9
#define PLACAR_CARROS_1ANDAR         10
#define PLACAR_CARROS_2ANDAR         11
#define PLACAR_FLAGS                 12

// Estrutura para dados da câmera LPR
typedef struct {
    uint8_t status;
    char placa[9];  // 8 chars + '\0'
    uint8_t confianca;
    uint8_t erro;
} lpr_data_t;

// Estrutura para dados do placar
typedef struct {
    uint16_t vagas_terreo_pne;
    uint16_t vagas_terreo_idoso;
    uint16_t vagas_terreo_comuns;
    uint16_t vagas_1andar_pne;
    uint16_t vagas_1andar_idoso;
    uint16_t vagas_1andar_comuns;
    uint16_t vagas_2andar_pne;
    uint16_t vagas_2andar_idoso;
    uint16_t vagas_2andar_comuns;
    uint16_t carros_terreo;
    uint16_t carros_1andar;
    uint16_t carros_2andar;
    uint16_t flags;
} placar_data_t;

/**
 * @brief Dispara a captura de placa na câmera LPR
 * @param uart_fd File descriptor da UART
 * @param camera_addr Endereço da câmera (0x11 ou 0x12)
 * @param matricula Últimos 4 dígitos da matrícula
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int lpr_trigger_capture(int uart_fd, uint8_t camera_addr, const char *matricula);

/**
 * @brief Lê o status da câmera LPR
 * @param uart_fd File descriptor da UART
 * @param camera_addr Endereço da câmera (0x11 ou 0x12)
 * @param matricula Últimos 4 dígitos da matrícula
 * @param status Ponteiro para armazenar o status
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int lpr_read_status(int uart_fd, uint8_t camera_addr, const char *matricula, uint8_t *status);

/**
 * @brief Lê os dados completos da câmera LPR (placa e confiança)
 * @param uart_fd File descriptor da UART
 * @param camera_addr Endereço da câmera (0x11 ou 0x12)
 * @param matricula Últimos 4 dígitos da matrícula
 * @param data Ponteiro para estrutura lpr_data_t
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int lpr_read_data(int uart_fd, uint8_t camera_addr, const char *matricula, lpr_data_t *data);

/**
 * @brief Zera o trigger da câmera LPR
 * @param uart_fd File descriptor da UART
 * @param camera_addr Endereço da câmera (0x11 ou 0x12)
 * @param matricula Últimos 4 dígitos da matrícula
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int lpr_reset_trigger(int uart_fd, uint8_t camera_addr, const char *matricula);

/**
 * @brief Atualiza os dados do placar de vagas
 * @param uart_fd File descriptor da UART
 * @param matricula Últimos 4 dígitos da matrícula
 * @param data Ponteiro para estrutura placar_data_t com os dados a escrever
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int placar_update(int uart_fd, const char *matricula, const placar_data_t *data);

/**
 * @brief Função auxiliar para imprimir buffer (debug)
 * @param buffer Buffer a imprimir
 * @param len Tamanho do buffer
 */
void print_buffer(const uint8_t *buffer, int len);

/**
 * @brief Função de alto nível para capturar placa com retry
 * @param uart_fd File descriptor da UART
 * @param camera_addr Endereço da câmera (0x11 ou 0x12)
 * @param matricula Últimos 4 dígitos da matrícula
 * @param data Ponteiro para estrutura lpr_data_t
 * @param max_retries Número máximo de tentativas
 * @param timeout_ms Timeout em milissegundos para polling
 * @return 0 em caso de sucesso, -1 em caso de erro
 */
int lpr_capture_plate(int uart_fd, uint8_t camera_addr, const char *matricula, 
                      lpr_data_t *data, int max_retries, int timeout_ms);

#endif
