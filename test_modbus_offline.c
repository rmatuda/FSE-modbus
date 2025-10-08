#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "modbus_parking.h"
#include "crc16.h"

// Matrícula do aluno
#define MATRICULA "6383"

// Função para simular construção de mensagem MODBUS (cópia da função interna)
static int build_modbus_message_test(uint8_t *buffer, uint8_t addr, uint8_t func, 
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

void test_trigger_capture_message() {
    printf("\n========== TESTE: TRIGGER CAPTURE MESSAGE ==========\n");
    
    uint8_t tx_buffer[32];
    uint8_t data[] = {
        0x01, 0x00,  // Starting Address (offset 1) - little-endian
        0x01, 0x00,  // Quantity of Registers (1) - little-endian
        0x02,        // Byte Count (2 bytes)
        0x01, 0x00   // Register Value (1 = trigger) - little-endian
    };
    
    int tx_len = build_modbus_message_test(tx_buffer, CAMERA_ENTRADA_ADDR, MODBUS_WRITE_MULTIPLE_REGS, 
                                           data, sizeof(data), MATRICULA);
    
    printf("Câmera: 0x%02X (ENTRADA)\n", CAMERA_ENTRADA_ADDR);
    printf("Função: 0x%02X (WRITE MULTIPLE REGISTERS)\n", MODBUS_WRITE_MULTIPLE_REGS);
    printf("Matrícula: %s\n", MATRICULA);
    printf("Tamanho da mensagem: %d bytes\n", tx_len);
    printf("Dados enviados: ");
    for (int i = 0; i < tx_len; i++) {
        printf("0x%02X ", tx_buffer[i]);
    }
    printf("\n\nDecodificação:\n");
    printf("  [0] Endereço: 0x%02X\n", tx_buffer[0]);
    printf("  [1] Função: 0x%02X\n", tx_buffer[1]);
    printf("  [2-3] Start Address: 0x%02X%02X (little-endian = %d)\n", 
           tx_buffer[3], tx_buffer[2], tx_buffer[2] | (tx_buffer[3] << 8));
    printf("  [4-5] Quantity: 0x%02X%02X (little-endian = %d)\n", 
           tx_buffer[5], tx_buffer[4], tx_buffer[4] | (tx_buffer[5] << 8));
    printf("  [6] Byte Count: %d\n", tx_buffer[6]);
    printf("  [7-8] Value: 0x%02X%02X (little-endian = %d)\n", 
           tx_buffer[8], tx_buffer[7], tx_buffer[7] | (tx_buffer[8] << 8));
    printf("  [9-12] Matrícula: '%c%c%c%c'\n", 
           tx_buffer[9], tx_buffer[10], tx_buffer[11], tx_buffer[12]);
    
    uint16_t received_crc = tx_buffer[13] | (tx_buffer[14] << 8);
    uint16_t calculated_crc = crc16_modbus(tx_buffer, tx_len - 2);
    printf("  [13-14] CRC: 0x%04X (calculado: 0x%04X) %s\n", 
           received_crc, calculated_crc, 
           (received_crc == calculated_crc) ? "✓ OK" : "✗ ERRO");
}

void test_read_status_message() {
    printf("\n========== TESTE: READ STATUS MESSAGE ==========\n");
    
    uint8_t tx_buffer[32];
    uint8_t data[] = {
        0x00, 0x00,  // Starting Address (offset 0 = Status) - little-endian  
        0x01, 0x00   // Quantity of Registers (1) - little-endian
    };
    
    int tx_len = build_modbus_message_test(tx_buffer, CAMERA_ENTRADA_ADDR, MODBUS_READ_HOLDING_REGS, 
                                           data, sizeof(data), MATRICULA);
    
    printf("Câmera: 0x%02X (ENTRADA)\n", CAMERA_ENTRADA_ADDR);
    printf("Função: 0x%02X (READ HOLDING REGISTERS)\n", MODBUS_READ_HOLDING_REGS);
    printf("Tamanho da mensagem: %d bytes\n", tx_len);
    printf("Dados enviados: ");
    for (int i = 0; i < tx_len; i++) {
        printf("0x%02X ", tx_buffer[i]);
    }
    printf("\n\nDecodificação:\n");
    printf("  [0] Endereço: 0x%02X\n", tx_buffer[0]);
    printf("  [1] Função: 0x%02X\n", tx_buffer[1]);
    printf("  [2-3] Start Address: 0x%02X%02X (little-endian = %d)\n", 
           tx_buffer[3], tx_buffer[2], tx_buffer[2] | (tx_buffer[3] << 8));
    printf("  [4-5] Quantity: 0x%02X%02X (little-endian = %d)\n", 
           tx_buffer[5], tx_buffer[4], tx_buffer[4] | (tx_buffer[5] << 8));
    printf("  [6-9] Matrícula: '%c%c%c%c'\n", 
           tx_buffer[6], tx_buffer[7], tx_buffer[8], tx_buffer[9]);
    
    uint16_t received_crc = tx_buffer[10] | (tx_buffer[11] << 8);
    uint16_t calculated_crc = crc16_modbus(tx_buffer, tx_len - 2);
    printf("  [10-11] CRC: 0x%04X (calculado: 0x%04X) %s\n", 
           received_crc, calculated_crc, 
           (received_crc == calculated_crc) ? "✓ OK" : "✗ ERRO");
}

void test_placar_message() {
    printf("\n========== TESTE: PLACAR UPDATE MESSAGE ==========\n");
    
    uint8_t tx_buffer[64];
    
    // Dados de exemplo
    placar_data_t placar = {
        .vagas_terreo_pne = 2,
        .vagas_terreo_idoso = 3,
        .vagas_terreo_comuns = 10,
        .vagas_1andar_pne = 1,
        .vagas_1andar_idoso = 2,
        .vagas_1andar_comuns = 8,
        .vagas_2andar_pne = 0,
        .vagas_2andar_idoso = 1,
        .vagas_2andar_comuns = 5,
        .carros_terreo = 5,
        .carros_1andar = 9,
        .carros_2andar = 14,
        .flags = 0x04
    };
    
    // Constrói dados da requisição
    uint8_t req_data[31]; // 5 bytes cabeçalho + 26 bytes dados
    req_data[0] = 0x00; // Starting Address Lo
    req_data[1] = 0x00; // Starting Address Hi (offset 0) - little-endian
    req_data[2] = 0x0D; // Quantity of Registers Lo
    req_data[3] = 0x00; // Quantity of Registers Hi (13) - little-endian
    req_data[4] = 0x1A; // Byte Count (26 bytes = 13 regs * 2)
    
    // Preenche dados (little-endian)
    int idx = 5;
    req_data[idx++] = placar.vagas_terreo_pne & 0xFF;
    req_data[idx++] = (placar.vagas_terreo_pne >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_terreo_idoso & 0xFF;
    req_data[idx++] = (placar.vagas_terreo_idoso >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_terreo_comuns & 0xFF;
    req_data[idx++] = (placar.vagas_terreo_comuns >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_1andar_pne & 0xFF;
    req_data[idx++] = (placar.vagas_1andar_pne >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_1andar_idoso & 0xFF;
    req_data[idx++] = (placar.vagas_1andar_idoso >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_1andar_comuns & 0xFF;
    req_data[idx++] = (placar.vagas_1andar_comuns >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_2andar_pne & 0xFF;
    req_data[idx++] = (placar.vagas_2andar_pne >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_2andar_idoso & 0xFF;
    req_data[idx++] = (placar.vagas_2andar_idoso >> 8) & 0xFF;
    req_data[idx++] = placar.vagas_2andar_comuns & 0xFF;
    req_data[idx++] = (placar.vagas_2andar_comuns >> 8) & 0xFF;
    req_data[idx++] = placar.carros_terreo & 0xFF;
    req_data[idx++] = (placar.carros_terreo >> 8) & 0xFF;
    req_data[idx++] = placar.carros_1andar & 0xFF;
    req_data[idx++] = (placar.carros_1andar >> 8) & 0xFF;
    req_data[idx++] = placar.carros_2andar & 0xFF;
    req_data[idx++] = (placar.carros_2andar >> 8) & 0xFF;
    req_data[idx++] = placar.flags & 0xFF;
    req_data[idx++] = (placar.flags >> 8) & 0xFF;
    
    int tx_len = build_modbus_message_test(tx_buffer, PLACAR_VAGAS_ADDR, MODBUS_WRITE_MULTIPLE_REGS, 
                                           req_data, sizeof(req_data), MATRICULA);
    
    printf("Placar: 0x%02X\n", PLACAR_VAGAS_ADDR);
    printf("Função: 0x%02X (WRITE MULTIPLE REGISTERS)\n", MODBUS_WRITE_MULTIPLE_REGS);
    printf("Tamanho da mensagem: %d bytes\n", tx_len);
    printf("Dados enviados (primeiros 20 bytes): ");
    for (int i = 0; i < (tx_len > 20 ? 20 : tx_len); i++) {
        printf("0x%02X ", tx_buffer[i]);
    }
    if (tx_len > 20) printf("...");
    printf("\n\nDecodificação dos dados do placar (little-endian):\n");
    printf("  Térreo PNE: %d (bytes: 0x%02X 0x%02X)\n", placar.vagas_terreo_pne, req_data[5], req_data[6]);
    printf("  Térreo Idoso: %d (bytes: 0x%02X 0x%02X)\n", placar.vagas_terreo_idoso, req_data[7], req_data[8]);
    printf("  Térreo Comuns: %d (bytes: 0x%02X 0x%02X)\n", placar.vagas_terreo_comuns, req_data[9], req_data[10]);
    printf("  Flags: 0x%04X (bytes: 0x%02X 0x%02X)\n", placar.flags, req_data[29], req_data[30]);
    
    uint16_t received_crc = tx_buffer[tx_len-2] | (tx_buffer[tx_len-1] << 8);
    uint16_t calculated_crc = crc16_modbus(tx_buffer, tx_len - 2);
    printf("  CRC: 0x%04X (calculado: 0x%04X) %s\n", 
           received_crc, calculated_crc, 
           (received_crc == calculated_crc) ? "✓ OK" : "✗ ERRO");
}

int main() {
    printf("=================================================\n");
    printf("  TESTE OFFLINE DAS MENSAGENS MODBUS\n");
    printf("=================================================\n");
    printf("Matrícula: %s\n", MATRICULA);
    printf("=================================================\n");
    
    test_trigger_capture_message();
    test_read_status_message();
    test_placar_message();
    
    printf("\n=================================================\n");
    printf("  TESTE CONCLUÍDO\n");
    printf("=================================================\n");
    
    return 0;
}
