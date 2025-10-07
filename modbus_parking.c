#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "modbus_parking.h"
#include "crc16.h"
#include "uart.h"

// Função auxiliar para construir mensagem MODBUS com matrícula
static int build_modbus_message(uint8_t *buffer, uint8_t addr, uint8_t func, 
                                 const uint8_t *data, int data_len, const char *matricula) {
    uint8_t *ptr = buffer;
    
    // Endereço do dispositivo
    *ptr++ = addr;
    
    // Código de função
    *ptr++ = func;
    
    // Dados da requisição
    if (data != NULL && data_len > 0) {
        memcpy(ptr, data, data_len);
        ptr += data_len;
    }
    
    // Adiciona os 4 últimos dígitos da matrícula
    for (int i = 0; i < 4; i++) {
        *ptr++ = (uint8_t)matricula[i];
    }
    
    // Calcula e adiciona o CRC
    int msg_len = ptr - buffer;
    uint16_t crc = crc16_modbus(buffer, msg_len);
    *ptr++ = crc & 0xFF;        // CRC Low
    *ptr++ = (crc >> 8) & 0xFF; // CRC High
    
    return ptr - buffer;
}

// Função auxiliar para verificar resposta MODBUS
static int verify_modbus_response(const uint8_t *buffer, int len, uint8_t expected_addr, uint8_t expected_func) {
    if (len < 5) {  // Mínimo: addr + func + 1 byte data + 2 bytes CRC
        printf("Resposta muito curta: %d bytes\n", len);
        return -1;
    }
    
    // Verifica CRC
    uint16_t received_crc = buffer[len-2] | (buffer[len-1] << 8);
    uint16_t calculated_crc = crc16_modbus(buffer, len - 2);
    
    if (received_crc != calculated_crc) {
        printf("Erro de CRC: recebido=0x%04X, calculado=0x%04X\n", received_crc, calculated_crc);
        return -1;
    }
    
    // Verifica endereço e função
    if (buffer[0] != expected_addr) {
        printf("Endereço incorreto: esperado=0x%02X, recebido=0x%02X\n", expected_addr, buffer[0]);
        return -1;
    }
    
    if (buffer[1] != expected_func) {
        // Verifica se é uma exceção MODBUS (função | 0x80)
        if (buffer[1] == (expected_func | 0x80)) {
            printf("Exceção MODBUS: código=0x%02X\n", buffer[2]);
            return -1;
        }
        printf("Função incorreta: esperado=0x%02X, recebido=0x%02X\n", expected_func, buffer[1]);
        return -1;
    }
    
    return 0;
}

int lpr_trigger_capture(int uart_fd, uint8_t camera_addr, const char *matricula) {
    uint8_t tx_buffer[32];
    uint8_t rx_buffer[32];
    
    // Prepara dados: Write Single Register (0x06) ou Write Multiple Registers (0x10)
    // Usando 0x10 para escrever no offset 1 (Trigger)
    uint8_t data[] = {
        0x00, 0x01,  // Starting Address (offset 1)
        0x00, 0x01,  // Quantity of Registers (1)
        0x02,        // Byte Count (2 bytes)
        0x00, 0x01   // Register Value (1 = trigger)
    };
    
    int tx_len = build_modbus_message(tx_buffer, camera_addr, MODBUS_WRITE_MULTIPLE_REGS, 
                                      data, sizeof(data), matricula);
    
    printf("Enviando trigger para câmera 0x%02X...\n", camera_addr);
    print_buffer(tx_buffer, tx_len);
    
    send_uart(uart_fd, tx_buffer, tx_len);
    usleep(50000); // 50ms delay
    
    int rx_len = receive_uart(uart_fd, rx_buffer, sizeof(rx_buffer));
    if (rx_len > 0) {
        print_buffer(rx_buffer, rx_len);
        return verify_modbus_response(rx_buffer, rx_len, camera_addr, MODBUS_WRITE_MULTIPLE_REGS);
    }
    
    printf("Timeout: nenhuma resposta recebida\n");
    return -1;
}

int lpr_read_status(int uart_fd, uint8_t camera_addr, const char *matricula, uint8_t *status) {
    uint8_t tx_buffer[32];
    uint8_t rx_buffer[32];
    
    // Read Holding Registers: offset 0, quantidade 1
    uint8_t data[] = {
        0x00, 0x00,  // Starting Address (offset 0 = Status)
        0x00, 0x01   // Quantity of Registers (1)
    };
    
    int tx_len = build_modbus_message(tx_buffer, camera_addr, MODBUS_READ_HOLDING_REGS, 
                                      data, sizeof(data), matricula);
    
    send_uart(uart_fd, tx_buffer, tx_len);
    usleep(50000); // 50ms delay
    
    int rx_len = receive_uart(uart_fd, rx_buffer, sizeof(rx_buffer));
    if (rx_len > 0) {
        if (verify_modbus_response(rx_buffer, rx_len, camera_addr, MODBUS_READ_HOLDING_REGS) == 0) {
            // Formato resposta: [addr][func][byte_count][data_hi][data_lo][crc_lo][crc_hi]
            *status = rx_buffer[4]; // Low byte do registrador
            return 0;
        }
    }
    
    return -1;
}

int lpr_read_data(int uart_fd, uint8_t camera_addr, const char *matricula, lpr_data_t *data) {
    uint8_t tx_buffer[32];
    uint8_t rx_buffer[64];
    
    // Read Holding Registers: offset 0, quantidade 8 (status + trigger + placa[4] + confiança + erro)
    uint8_t req_data[] = {
        0x00, 0x00,  // Starting Address (offset 0)
        0x00, 0x08   // Quantity of Registers (8)
    };
    
    int tx_len = build_modbus_message(tx_buffer, camera_addr, MODBUS_READ_HOLDING_REGS, 
                                      req_data, sizeof(req_data), matricula);
    
    printf("Lendo dados da câmera 0x%02X...\n", camera_addr);
    print_buffer(tx_buffer, tx_len);
    
    send_uart(uart_fd, tx_buffer, tx_len);
    usleep(50000); // 50ms delay
    
    int rx_len = receive_uart(uart_fd, rx_buffer, sizeof(rx_buffer));
    if (rx_len > 0) {
        print_buffer(rx_buffer, rx_len);
        
        if (verify_modbus_response(rx_buffer, rx_len, camera_addr, MODBUS_READ_HOLDING_REGS) == 0) {
            // Formato resposta: [addr][func][byte_count][data...][crc_lo][crc_hi]
            int byte_count = rx_buffer[2];
            
            if (byte_count >= 16) { // 8 registradores * 2 bytes
                // Status (offset 0)
                data->status = rx_buffer[4]; // Low byte
                
                // Placa (offset 2-5): 4 registradores, 2 bytes cada = 8 chars
                for (int i = 0; i < 8; i++) {
                    data->placa[i] = rx_buffer[7 + i]; // Começa no offset 2 (byte 7)
                }
                data->placa[8] = '\0';
                
                // Confiança (offset 6)
                data->confianca = rx_buffer[15]; // Low byte
                
                // Erro (offset 7)
                data->erro = rx_buffer[17]; // Low byte
                
                return 0;
            }
        }
    }
    
    return -1;
}

int lpr_reset_trigger(int uart_fd, uint8_t camera_addr, const char *matricula) {
    uint8_t tx_buffer[32];
    uint8_t rx_buffer[32];
    
    // Write Multiple Registers: escrever 0 no offset 1 (Trigger)
    uint8_t data[] = {
        0x00, 0x01,  // Starting Address (offset 1)
        0x00, 0x01,  // Quantity of Registers (1)
        0x02,        // Byte Count (2 bytes)
        0x00, 0x00   // Register Value (0 = reset)
    };
    
    int tx_len = build_modbus_message(tx_buffer, camera_addr, MODBUS_WRITE_MULTIPLE_REGS, 
                                      data, sizeof(data), matricula);
    
    send_uart(uart_fd, tx_buffer, tx_len);
    usleep(50000); // 50ms delay
    
    int rx_len = receive_uart(uart_fd, rx_buffer, sizeof(rx_buffer));
    if (rx_len > 0) {
        return verify_modbus_response(rx_buffer, rx_len, camera_addr, MODBUS_WRITE_MULTIPLE_REGS);
    }
    
    return -1;
}

int placar_update(int uart_fd, const char *matricula, const placar_data_t *data) {
    uint8_t tx_buffer[64];
    uint8_t rx_buffer[32];
    
    // Write Multiple Registers: escrever 13 registradores (offset 0-12)
    uint8_t req_data[30];
    req_data[0] = 0x00; // Starting Address Hi
    req_data[1] = 0x00; // Starting Address Lo (offset 0)
    req_data[2] = 0x00; // Quantity of Registers Hi
    req_data[3] = 0x0D; // Quantity of Registers Lo (13)
    req_data[4] = 0x1A; // Byte Count (26 bytes = 13 regs * 2)
    
    // Preenche os dados dos registradores (big-endian)
    int idx = 5;
    req_data[idx++] = (data->vagas_terreo_pne >> 8) & 0xFF;
    req_data[idx++] = data->vagas_terreo_pne & 0xFF;
    req_data[idx++] = (data->vagas_terreo_idoso >> 8) & 0xFF;
    req_data[idx++] = data->vagas_terreo_idoso & 0xFF;
    req_data[idx++] = (data->vagas_terreo_comuns >> 8) & 0xFF;
    req_data[idx++] = data->vagas_terreo_comuns & 0xFF;
    req_data[idx++] = (data->vagas_1andar_pne >> 8) & 0xFF;
    req_data[idx++] = data->vagas_1andar_pne & 0xFF;
    req_data[idx++] = (data->vagas_1andar_idoso >> 8) & 0xFF;
    req_data[idx++] = data->vagas_1andar_idoso & 0xFF;
    req_data[idx++] = (data->vagas_1andar_comuns >> 8) & 0xFF;
    req_data[idx++] = data->vagas_1andar_comuns & 0xFF;
    req_data[idx++] = (data->vagas_2andar_pne >> 8) & 0xFF;
    req_data[idx++] = data->vagas_2andar_pne & 0xFF;
    req_data[idx++] = (data->vagas_2andar_idoso >> 8) & 0xFF;
    req_data[idx++] = data->vagas_2andar_idoso & 0xFF;
    req_data[idx++] = (data->vagas_2andar_comuns >> 8) & 0xFF;
    req_data[idx++] = data->vagas_2andar_comuns & 0xFF;
    req_data[idx++] = (data->carros_terreo >> 8) & 0xFF;
    req_data[idx++] = data->carros_terreo & 0xFF;
    req_data[idx++] = (data->carros_1andar >> 8) & 0xFF;
    req_data[idx++] = data->carros_1andar & 0xFF;
    req_data[idx++] = (data->carros_2andar >> 8) & 0xFF;
    req_data[idx++] = data->carros_2andar & 0xFF;
    req_data[idx++] = (data->flags >> 8) & 0xFF;
    req_data[idx++] = data->flags & 0xFF;
    
    int tx_len = build_modbus_message(tx_buffer, PLACAR_VAGAS_ADDR, MODBUS_WRITE_MULTIPLE_REGS, 
                                      req_data, sizeof(req_data), matricula);
    
    printf("Atualizando placar de vagas...\n");
    print_buffer(tx_buffer, tx_len);
    
    send_uart(uart_fd, tx_buffer, tx_len);
    usleep(50000); // 50ms delay
    
    int rx_len = receive_uart(uart_fd, rx_buffer, sizeof(rx_buffer));
    if (rx_len > 0) {
        print_buffer(rx_buffer, rx_len);
        return verify_modbus_response(rx_buffer, rx_len, PLACAR_VAGAS_ADDR, MODBUS_WRITE_MULTIPLE_REGS);
    }
    
    return -1;
}

void print_buffer(const uint8_t *buffer, int len) {
    printf("Buffer (%d bytes): ", len);
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", buffer[i]);
    }
    printf("\n");
}

int lpr_capture_plate(int uart_fd, uint8_t camera_addr, const char *matricula, 
                      lpr_data_t *data, int max_retries, int timeout_ms) {
    int retry = 0;
    
    while (retry < max_retries) {
        printf("\n=== Tentativa %d/%d ===\n", retry + 1, max_retries);
        
        // 1. Disparar trigger
        if (lpr_trigger_capture(uart_fd, camera_addr, matricula) != 0) {
            printf("Erro ao disparar trigger\n");
            retry++;
            usleep(100000 * (1 << retry)); // Backoff exponencial
            continue;
        }
        
        // 2. Polling do status
        int poll_count = 0;
        int max_polls = timeout_ms / 100; // Poll a cada 100ms
        uint8_t status = LPR_STATUS_PRONTO;
        
        while (poll_count < max_polls) {
            if (lpr_read_status(uart_fd, camera_addr, matricula, &status) == 0) {
                printf("Status: %d\n", status);
                
                if (status == LPR_STATUS_OK) {
                    // 3. Ler dados completos
                    if (lpr_read_data(uart_fd, camera_addr, matricula, data) == 0) {
                        printf("Placa capturada: %s (confiança: %d%%)\n", data->placa, data->confianca);
                        
                        // 4. Resetar trigger
                        lpr_reset_trigger(uart_fd, camera_addr, matricula);
                        
                        return 0;
                    }
                } else if (status == LPR_STATUS_ERRO) {
                    printf("Erro na captura\n");
                    break;
                }
            }
            
            usleep(100000); // 100ms
            poll_count++;
        }
        
        retry++;
        if (retry < max_retries) {
            int backoff = 100000 * (1 << retry); // Backoff exponencial
            printf("Aguardando %d ms antes de tentar novamente...\n", backoff / 1000);
            usleep(backoff);
        }
    }
    
    printf("Falha após %d tentativas\n", max_retries);
    return -1;
}
