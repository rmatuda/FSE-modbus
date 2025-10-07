# Biblioteca MODBUS para Sistema de Estacionamento

Esta biblioteca implementa a comunicação MODBUS RTU via RS485 para o sistema de controle de estacionamento, adaptada do projeto RaioX.

## 📋 Características

- **Comunicação RS485-MODBUS RTU** (115200 bps, 8N1)
- **Suporte a dispositivos do estacionamento:**
  - Câmera LPR de Entrada (0x11)
  - Câmera LPR de Saída (0x12)
  - Placar de Vagas (0x20)
- **Funções MODBUS implementadas:**
  - 0x03 - Read Holding Registers
  - 0x10 - Write Multiple Registers
- **Recursos avançados:**
  - Cálculo automático de CRC16
  - Retry com backoff exponencial
  - Timeout configurável
  - Validação de respostas
  - Inserção automática de matrícula

## 📁 Estrutura de Arquivos

```
modbus/
├── crc16.h              # Header do CRC16
├── crc16.c              # Implementação do CRC16 MODBUS
├── uart.h               # Header da comunicação UART
├── uart.c               # Implementação da UART (RS485)
├── modbus_parking.h     # Header principal da biblioteca
├── modbus_parking.c     # Implementação das funções MODBUS
├── example_parking.c    # Exemplo de uso
├── Makefile             # Compilação
└── README.md            # Esta documentação
```

## 🔧 Compilação

### Compilar a biblioteca e exemplo:

```bash
cd modbus
make
```

### Limpar arquivos compilados:

```bash
make clean
```

### Instalar biblioteca (copia para ../lib e ../include):

```bash
make install
```

## 🚀 Uso Básico

### 1. Incluir os headers necessários:

```c
#include "modbus_parking.h"
#include "uart.h"
```

### 2. Abrir a porta UART:

```c
int uart_fd = open_uart("/dev/ttyUSB0");
if (uart_fd < 0) {
    fprintf(stderr, "Erro ao abrir UART\n");
    return -1;
}
```

### 3. Usar as funções da biblioteca:

#### Capturar placa da câmera de entrada:

```c
lpr_data_t data;
const char *matricula = "6383"; // Últimos 4 dígitos da sua matrícula

if (lpr_capture_plate(uart_fd, CAMERA_ENTRADA_ADDR, matricula, &data, 3, 2000) == 0) {
    printf("Placa: %s\n", data.placa);
    printf("Confiança: %d%%\n", data.confianca);
}
```

#### Atualizar placar de vagas:

```c
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

placar_update(uart_fd, matricula, &placar);
```

### 4. Fechar a UART:

```c
close_uart(uart_fd);
```

## 📚 API da Biblioteca

### Funções de Câmera LPR

#### `lpr_capture_plate()`
Função de alto nível para capturar placa com retry automático.

```c
int lpr_capture_plate(int uart_fd, uint8_t camera_addr, const char *matricula, 
                      lpr_data_t *data, int max_retries, int timeout_ms);
```

**Parâmetros:**
- `uart_fd`: File descriptor da UART
- `camera_addr`: Endereço da câmera (0x11 ou 0x12)
- `matricula`: Últimos 4 dígitos da matrícula
- `data`: Ponteiro para estrutura lpr_data_t
- `max_retries`: Número máximo de tentativas (recomendado: 3)
- `timeout_ms`: Timeout em ms para polling (recomendado: 2000)

**Retorno:** 0 em sucesso, -1 em erro

#### `lpr_trigger_capture()`
Dispara a captura de placa.

```c
int lpr_trigger_capture(int uart_fd, uint8_t camera_addr, const char *matricula);
```

#### `lpr_read_status()`
Lê o status da câmera.

```c
int lpr_read_status(int uart_fd, uint8_t camera_addr, const char *matricula, uint8_t *status);
```

**Status possíveis:**
- `LPR_STATUS_PRONTO` (0): Pronto
- `LPR_STATUS_PROCESSANDO` (1): Processando
- `LPR_STATUS_OK` (2): OK
- `LPR_STATUS_ERRO` (3): Erro

#### `lpr_read_data()`
Lê todos os dados da câmera (placa, confiança, etc).

```c
int lpr_read_data(int uart_fd, uint8_t camera_addr, const char *matricula, lpr_data_t *data);
```

#### `lpr_reset_trigger()`
Reseta o trigger da câmera.

```c
int lpr_reset_trigger(int uart_fd, uint8_t camera_addr, const char *matricula);
```

### Funções do Placar

#### `placar_update()`
Atualiza todos os dados do placar de vagas.

```c
int placar_update(int uart_fd, const char *matricula, const placar_data_t *data);
```

### Estruturas de Dados

#### `lpr_data_t`
```c
typedef struct {
    uint8_t status;      // Status da câmera
    char placa[9];       // Placa (8 chars + '\0')
    uint8_t confianca;   // Confiança (0-100%)
    uint8_t erro;        // Código de erro
} lpr_data_t;
```

#### `placar_data_t`
```c
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
    uint16_t flags;  // bit0=lotado geral, bit1=lotado 1º, bit2=lotado 2º
} placar_data_t;
```

## 🧪 Executar Exemplo

### Compilar:
```bash
make
```

### Executar (padrão /dev/ttyUSB0):
```bash
./example_parking
```

### Executar com dispositivo específico:
```bash
./example_parking /dev/ttyUSB1
```

### Menu interativo:
```
=================================================
MENU DE TESTES:
1 - Testar Câmera de Entrada (0x11)
2 - Testar Câmera de Saída (0x12)
3 - Testar Placar de Vagas (0x20)
4 - Executar todos os testes
0 - Sair
=================================================
```

## 🔍 Fluxo de Operação - Câmera LPR

### Fluxo típico de captura de placa:

1. **Trigger**: Escreve 1 no registrador de trigger (offset 1)
2. **Polling**: Lê status (offset 0) até receber OK (2) ou ERRO (3)
3. **Leitura**: Se OK, lê placa (offset 2-5) e confiança (offset 6)
4. **Reset**: Escreve 0 no trigger para resetar

### Exemplo de uso no fluxo de entrada:

```c
// Quando sensor de presença detecta veículo
lpr_data_t entrada_data;

if (lpr_capture_plate(uart_fd, CAMERA_ENTRADA_ADDR, MATRICULA, 
                      &entrada_data, 3, 2000) == 0) {
    if (entrada_data.confianca >= 70) {
        // Envia evento para servidor central
        enviar_evento_entrada(entrada_data.placa, entrada_data.confianca);
        // Abre cancela
        abrir_cancela_entrada();
    } else {
        // Confiança baixa - gerar ticket temporário
        gerar_ticket_temporario();
    }
}
```

## ⚙️ Configuração

### Parâmetros UART (uart.c):
- **Baudrate**: 115200 bps
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1
- **Timeout**: 500ms (configurável em `options.c_cc[VTIME]`)

### Endereços MODBUS:
- **Câmera Entrada**: 0x11
- **Câmera Saída**: 0x12
- **Placar**: 0x20

### Matrícula:
⚠️ **IMPORTANTE**: Alterar a constante `MATRICULA` em `example_parking.c` com os últimos 4 dígitos da sua matrícula!

```c
#define MATRICULA "6383"
```

## 🐛 Tratamento de Erros

A biblioteca implementa:

- **Validação de CRC**: Todas as respostas são verificadas
- **Retry com backoff exponencial**: 100ms, 250ms, 500ms
- **Timeout**: Configurável por requisição
- **Logs de debug**: Função `print_buffer()` para debug

### Códigos de retorno:
- `0`: Sucesso
- `-1`: Erro (timeout, CRC inválido, exceção MODBUS, etc)

## 📝 Integração com o Sistema

### No Servidor do Andar Térreo:

```c
// Inicialização
int modbus_fd = open_uart("/dev/ttyUSB0");

// Thread de entrada
void* entrada_task(void* arg) {
    while (running) {
        if (sensor_entrada_ativado()) {
            lpr_data_t data;
            if (lpr_capture_plate(modbus_fd, CAMERA_ENTRADA_ADDR, 
                                  MATRICULA, &data, 3, 2000) == 0) {
                processar_entrada(data.placa, data.confianca);
            }
        }
        usleep(100000);
    }
}

// Thread de saída
void* saida_task(void* arg) {
    while (running) {
        if (sensor_saida_ativado()) {
            lpr_data_t data;
            if (lpr_capture_plate(modbus_fd, CAMERA_SAIDA_ADDR, 
                                  MATRICULA, &data, 3, 2000) == 0) {
                processar_saida(data.placa, data.confianca);
            }
        }
        usleep(100000);
    }
}

// Atualização periódica do placar (a cada 1s ou quando houver mudança)
void atualizar_placar_periodico() {
    placar_data_t placar;
    // Preencher com dados do servidor central
    obter_dados_vagas(&placar);
    placar_update(modbus_fd, MATRICULA, &placar);
}
```

## 📖 Referências

- [MODBUS Application Protocol V1.1b3](https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
- Especificação do trabalho (README.md principal)
- Projeto RaioX (base da implementação)

## 🔗 Arquivos Relacionados

- `../README.md`: Especificação completa do trabalho
- `../figuras/`: Diagramas do sistema

## ✅ Checklist de Implementação

- [x] CRC16 MODBUS
- [x] Comunicação UART (RS485, 115200 8N1)
- [x] Read Holding Registers (0x03)
- [x] Write Multiple Registers (0x10)
- [x] Trigger câmera LPR
- [x] Polling de status
- [x] Leitura de placa e confiança
- [x] Reset de trigger
- [x] Atualização do placar
- [x] Retry com backoff exponencial
- [x] Validação de CRC
- [x] Inserção de matrícula
- [x] Exemplo de uso
- [x] Documentação

---

**Desenvolvido para o Trabalho 1 de Fundamentos de Sistemas Embarcados (2025/2)**
