# Adaptação do MODBUS do RaioX para o Sistema de Estacionamento

## 📋 Resumo da Adaptação

Este documento descreve como o código MODBUS do projeto RaioX foi adaptado para o sistema de controle de estacionamento.

## 🔄 Mudanças Principais

### 1. Estrutura de Pacotes MODBUS

#### **RaioX (Original)**
```c
typedef struct {
    uint8_t addr;
    uint8_t code;
    uint8_t subcode;
    uint8_t offset;
    uint8_t *data;
    uint8_t *matr;
} mb_package;
```

#### **Estacionamento (Adaptado)**
- Removida a estrutura genérica `mb_package`
- Criadas funções específicas para cada dispositivo:
  - `lpr_trigger_capture()` - Dispara captura na câmera
  - `lpr_read_status()` - Lê status da câmera
  - `lpr_read_data()` - Lê placa e confiança
  - `placar_update()` - Atualiza placar de vagas

### 2. Funções MODBUS Implementadas

#### **RaioX**
- Função genérica `fill_buffer()` para qualquer tipo de mensagem

#### **Estacionamento**
- Função interna `build_modbus_message()` para construir mensagens
- Funções específicas para cada operação:
  - **Read Holding Registers (0x03)**: Leitura de status e dados das câmeras
  - **Write Multiple Registers (0x10)**: Escrita de trigger e atualização do placar

### 3. Dispositivos e Endereços

#### **RaioX**
- Dispositivos genéricos do sistema de Raio-X

#### **Estacionamento**
- **Câmera LPR Entrada**: 0x11
- **Câmera LPR Saída**: 0x12
- **Placar de Vagas**: 0x20

### 4. Mapa de Registradores

#### **Câmeras LPR (0x11 e 0x12)**

| Offset | Descrição | Tipo |
|--------|-----------|------|
| 0 | Status | u16 (0=Pronto, 1=Processando, 2=OK, 3=Erro) |
| 1 | Trigger Captura | u16 (0=Idle, 1=Disparar) |
| 2-5 | Placa (8 chars) | 4x u16 (ASCII) |
| 6 | Confiança (%) | u16 (0-100) |
| 7 | Erro | u16 |

#### **Placar de Vagas (0x20)**

| Offset | Descrição | Tipo |
|--------|-----------|------|
| 0 | Vagas Livres Térreo (PNE) | u16 |
| 1 | Vagas Livres Térreo (Idoso+) | u16 |
| 2 | Vagas Livres Térreo (Comuns) | u16 |
| 3 | Vagas Livres 1º Andar (PNE) | u16 |
| 4 | Vagas Livres 1º Andar (Idoso+) | u16 |
| 5 | Vagas Livres 1º Andar (Comuns) | u16 |
| 6 | Vagas Livres 2º Andar (PNE) | u16 |
| 7 | Vagas Livres 2º Andar (Idoso+) | u16 |
| 8 | Vagas Livres 2º Andar (Comuns) | u16 |
| 9 | Número de carros: Térreo | u16 |
| 10 | Número de carros: 1º Andar | u16 |
| 11 | Número de carros: 2º Andar | u16 |
| 12 | Flags (bit0=lotado geral, bit1=lotado 1º, bit2=lotado 2º) | u16 |

### 5. CRC16

#### **RaioX**
- Implementação com tabela lookup
- Função `CRC16()` e `get_crc()`

#### **Estacionamento**
- Mantida a implementação MODBUS padrão
- Função `crc16_modbus()` simplificada
- Validação automática de CRC nas respostas

### 6. UART/Serial

#### **RaioX**
- Configuração básica para `/dev/serial0`
- Timeout de 2s

#### **Estacionamento**
- Configuração para `/dev/ttyUSB0` (RS485)
- Parâmetros: **115200 bps, 8N1**
- Timeout ajustado para **500ms** (conforme especificação)
- Suporte a diferentes dispositivos via parâmetro

### 7. Tratamento de Erros e Retry

#### **RaioX**
- Sem retry automático

#### **Estacionamento**
- **Retry com backoff exponencial**: 100ms, 250ms, 500ms
- **Validação de respostas**: CRC, endereço, função
- **Detecção de exceções MODBUS**
- **Timeout configurável**
- **Logs de debug** com `print_buffer()`

### 8. Funções de Alto Nível

#### **Estacionamento (Novo)**

##### `lpr_capture_plate()`
Função completa para capturar placa com retry automático:

1. Dispara trigger
2. Faz polling do status até OK ou ERRO
3. Lê dados completos (placa + confiança)
4. Reseta trigger
5. Retry automático em caso de falha

```c
int lpr_capture_plate(int uart_fd, uint8_t camera_addr, const char *matricula, 
                      lpr_data_t *data, int max_retries, int timeout_ms);
```

### 9. Estruturas de Dados

#### **Estacionamento (Novo)**

```c
// Dados da câmera LPR
typedef struct {
    uint8_t status;
    char placa[9];
    uint8_t confianca;
    uint8_t erro;
} lpr_data_t;

// Dados do placar
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
```

## 📊 Comparação de Arquivos

### Arquivos Mantidos (com adaptações)

| RaioX | Estacionamento | Mudanças |
|-------|----------------|----------|
| `crc/crc16.h` | `crc16.h` | Simplificado |
| `crc/crc16.c` | `crc16.c` | Mantido algoritmo MODBUS |
| `src/uart.c` | `uart.c` | Ajustado para RS485, timeout 500ms |
| `include/modbus.h` | `modbus_parking.h` | Completamente redesenhado |
| `src/modbus.c` | `modbus_parking.c` | Funções específicas do estacionamento |

### Arquivos Novos

- `example_parking.c` - Exemplo de uso com menu interativo
- `README.md` - Documentação completa
- `ADAPTACAO.md` - Este documento
- `.env.example` - Configurações
- `Makefile` - Compilação

## 🔧 Melhorias Implementadas

### 1. **Modularização**
- Separação clara entre comunicação UART e protocolo MODBUS
- Funções específicas para cada dispositivo
- API de alto nível para facilitar uso

### 2. **Robustez**
- Validação completa de respostas
- Retry automático com backoff exponencial
- Detecção de exceções MODBUS
- Timeout configurável

### 3. **Usabilidade**
- Estruturas de dados tipadas
- Funções de alto nível (`lpr_capture_plate`)
- Exemplo completo com menu interativo
- Documentação detalhada

### 4. **Conformidade com Especificação**
- Implementa exatamente os registradores especificados
- Suporta os 3 dispositivos do barramento
- Insere matrícula conforme requisito
- Parâmetros de comunicação corretos (115200 8N1)

## 🚀 Como Usar no Projeto

### 1. Integração no Servidor do Andar Térreo

```c
#include "modbus_parking.h"
#include "uart.h"

// Inicialização
int modbus_fd = open_uart("/dev/ttyUSB0");

// Captura de entrada
void processar_entrada() {
    lpr_data_t data;
    if (lpr_capture_plate(modbus_fd, CAMERA_ENTRADA_ADDR, 
                          MATRICULA, &data, 3, 2000) == 0) {
        if (data.confianca >= 70) {
            enviar_evento_central(data.placa, data.confianca);
            abrir_cancela();
        }
    }
}

// Atualização do placar
void atualizar_placar(placar_data_t *dados) {
    placar_update(modbus_fd, MATRICULA, dados);
}
```

### 2. Compilação

```bash
cd modbus
make
```

### 3. Teste

```bash
./example_parking /dev/ttyUSB0
```

## ✅ Checklist de Conformidade

- [x] **CRC16 MODBUS** conforme especificação
- [x] **RS485 115200 8N1** configurado
- [x] **Timeout 200-500ms** implementado (500ms)
- [x] **Retry até 3 vezes** com backoff exponencial
- [x] **Matrícula inserida** antes do CRC
- [x] **Read Holding Registers (0x03)** implementado
- [x] **Write Multiple Registers (0x10)** implementado
- [x] **Câmera Entrada (0x11)** suportada
- [x] **Câmera Saída (0x12)** suportada
- [x] **Placar (0x20)** suportado
- [x] **Fluxo LPR completo**: trigger → polling → leitura → reset
- [x] **Tratamento de exceções MODBUS**
- [x] **Logs de erro** implementados

## 📝 Notas Importantes

### Diferenças Principais do RaioX

1. **Protocolo específico**: Ao invés de uma estrutura genérica, cada dispositivo tem suas funções
2. **Validação rigorosa**: Todas as respostas são validadas (CRC, endereço, função)
3. **Retry inteligente**: Backoff exponencial para evitar sobrecarga do barramento
4. **API amigável**: Funções de alto nível que encapsulam toda a complexidade

### Compatibilidade

- ✅ Compatível com especificação MODBUS RTU
- ✅ Compatível com requisitos do trabalho
- ✅ Testável com simuladores MODBUS
- ✅ Pronto para integração com Raspberry Pi

## 🔗 Referências

- Projeto RaioX original (base da adaptação)
- [MODBUS Application Protocol V1.1b3](https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)
- Especificação do Trabalho 1 (../README.md)

---

**Adaptação concluída com sucesso! ✅**
