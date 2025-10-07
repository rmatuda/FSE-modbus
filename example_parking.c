#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "modbus_parking.h"
#include "uart.h"

// Matrícula do aluno (últimos 4 dígitos)
#define MATRICULA "6383"

void test_camera_entrada(int uart_fd) {
    printf("\n========== TESTE CÂMERA DE ENTRADA (0x11) ==========\n");
    
    lpr_data_t data;
    
    // Captura placa com retry (máx 3 tentativas, timeout 2000ms)
    if (lpr_capture_plate(uart_fd, CAMERA_ENTRADA_ADDR, MATRICULA, &data, 3, 2000) == 0) {
        printf("\n✓ Captura bem-sucedida!\n");
        printf("  Placa: %s\n", data.placa);
        printf("  Confiança: %d%%\n", data.confianca);
        printf("  Status: %d\n", data.status);
        printf("  Erro: %d\n", data.erro);
    } else {
        printf("\n✗ Falha na captura da placa\n");
    }
}

void test_camera_saida(int uart_fd) {
    printf("\n========== TESTE CÂMERA DE SAÍDA (0x12) ==========\n");
    
    lpr_data_t data;
    
    // Captura placa com retry (máx 3 tentativas, timeout 2000ms)
    if (lpr_capture_plate(uart_fd, CAMERA_SAIDA_ADDR, MATRICULA, &data, 3, 2000) == 0) {
        printf("\n✓ Captura bem-sucedida!\n");
        printf("  Placa: %s\n", data.placa);
        printf("  Confiança: %d%%\n", data.confianca);
        printf("  Status: %d\n", data.status);
        printf("  Erro: %d\n", data.erro);
    } else {
        printf("\n✗ Falha na captura da placa\n");
    }
}

void test_placar(int uart_fd) {
    printf("\n========== TESTE PLACAR DE VAGAS (0x20) ==========\n");
    
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
        .flags = 0x04  // bit2 = 1 (2º andar lotado)
    };
    
    if (placar_update(uart_fd, MATRICULA, &placar) == 0) {
        printf("\n✓ Placar atualizado com sucesso!\n");
        printf("  Vagas livres Térreo: PNE=%d, Idoso=%d, Comuns=%d\n", 
               placar.vagas_terreo_pne, placar.vagas_terreo_idoso, placar.vagas_terreo_comuns);
        printf("  Vagas livres 1º Andar: PNE=%d, Idoso=%d, Comuns=%d\n", 
               placar.vagas_1andar_pne, placar.vagas_1andar_idoso, placar.vagas_1andar_comuns);
        printf("  Vagas livres 2º Andar: PNE=%d, Idoso=%d, Comuns=%d\n", 
               placar.vagas_2andar_pne, placar.vagas_2andar_idoso, placar.vagas_2andar_comuns);
        printf("  Carros: Térreo=%d, 1º=%d, 2º=%d\n", 
               placar.carros_terreo, placar.carros_1andar, placar.carros_2andar);
        printf("  Flags: 0x%04X\n", placar.flags);
    } else {
        printf("\n✗ Falha ao atualizar placar\n");
    }
}

int main(int argc, char *argv[]) {
    const char *uart_device = "/dev/serial0";
    
    // Permite especificar dispositivo UART via argumento
    if (argc > 1) {
        uart_device = argv[1];
    }
    
    printf("=================================================\n");
    printf("  TESTE DO SISTEMA MODBUS - ESTACIONAMENTO\n");
    printf("=================================================\n");
    printf("Dispositivo UART: %s\n", uart_device);
    printf("Matrícula: %s\n", MATRICULA);
    printf("=================================================\n");
    
    // Abre a UART
    int uart_fd = open_uart(uart_device);
    if (uart_fd < 0) {
        fprintf(stderr, "Erro ao abrir UART %s\n", uart_device);
        return 1;
    }
    
    printf("✓ UART aberta com sucesso (fd=%d)\n", uart_fd);
    
    // Menu interativo
    int opcao;
    do {
        printf("\n=================================================\n");
        printf("MENU DE TESTES:\n");
        printf("1 - Testar Câmera de Entrada (0x11)\n");
        printf("2 - Testar Câmera de Saída (0x12)\n");
        printf("3 - Testar Placar de Vagas (0x20)\n");
        printf("4 - Executar todos os testes\n");
        printf("0 - Sair\n");
        printf("=================================================\n");
        printf("Escolha uma opção: ");
        
        if (scanf("%d", &opcao) != 1) {
            printf("Opção inválida!\n");
            while (getchar() != '\n'); // Limpa buffer
            continue;
        }
        
        switch (opcao) {
            case 1:
                test_camera_entrada(uart_fd);
                break;
            case 2:
                test_camera_saida(uart_fd);
                break;
            case 3:
                test_placar(uart_fd);
                break;
            case 4:
                test_camera_entrada(uart_fd);
                sleep(1);
                test_camera_saida(uart_fd);
                sleep(1);
                test_placar(uart_fd);
                break;
            case 0:
                printf("\nEncerrando...\n");
                break;
            default:
                printf("Opção inválida!\n");
        }
        
    } while (opcao != 0);
    
    // Fecha a UART
    close_uart(uart_fd);
    printf("✓ UART fechada\n");
    
    return 0;
}
