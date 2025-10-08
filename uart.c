#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>
#include <sys/select.h>
#include "uart.h"

#define UART_DEVICE "/dev/serial0"

int open_uart(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == -1) {
        perror("Erro ao abrir a UART");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= CREAD | CLOCAL;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;
    
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 1;

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

void send_uart(int fd, const uint8_t *buffer, int len) {
    int total_written = 0;
    int bytes_written = 0;
    
    tcflush(fd, TCIFLUSH);
    
    while (total_written < len) {
        bytes_written = write(fd, buffer + total_written, len - total_written);
        
        if (bytes_written < 0) {
            perror("Erro ao escrever na UART");
            return;
        }
        
        total_written += bytes_written;
    }
    
    tcdrain(fd);
}

int receive_uart(int fd, uint8_t *buffer, int max_len) {
    int total_received = 0;
    int bytes_read = 0;
    fd_set read_fds;
    struct timeval timeout;
    int no_data_count = 0;
    const int MAX_NO_DATA = 3; 
    
    while (total_received < max_len) {
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        
        if (total_received == 0) {
            timeout.tv_sec = 1;      
            timeout.tv_usec = 500000; 
        } else {
            timeout.tv_sec = 0;      
            timeout.tv_usec = 200000;
        }
        
        int activity = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("Erro no select");
            return -1;
        }
        
        if (activity == 0) {
            // Timeout
            if (total_received == 0) {
                return 0;
            } else {
                no_data_count++;
                if (no_data_count >= MAX_NO_DATA) {
                    break;
                }
                continue;
            }
        }
        
        if (FD_ISSET(fd, &read_fds)) {
            bytes_read = read(fd, buffer + total_received, max_len - total_received);
            
            if (bytes_read < 0) {
                perror("Erro ao ler da UART");
                return -1;
            }
            
            if (bytes_read == 0) {
                break;
            }
            
            total_received += bytes_read;
            no_data_count = 0;
            
            // Para MODBUS, verificar se recebemos uma mensagem completa
            if (total_received >= 5) {
                // Mensagem MODBUS mínima: [addr][func][data][crc_lo][crc_hi]
                // Se é uma resposta de read holding registers, verificar byte count
                if (total_received >= 3 && buffer[1] == 0x03) {
                    int expected_length = buffer[2] + 5; // byte_count + addr + func + byte_count + 2 CRC
                    if (total_received >= expected_length) {
                        break; // Mensagem completa recebida
                    }
                }
                // Para outras funções MODBUS (write), geralmente 8 bytes
                else if (total_received >= 8) {
                    break; // Mensagem padrão recebida
                }
            }
        }
    }
    
    return total_received;
}

void close_uart(int fd) {
    close(fd);
}