#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdint.h>
#include "uart.h"

#define UART_DEVICE "/dev/ttyUSB0"

int open_uart(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Erro ao abrir a UART");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // Configuração: 115200 bps, 8N1
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag &= ~PARENB; // sem paridade
    options.c_cflag &= ~CSTOPB; // 1 stop bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;     // 8 bits
    options.c_cflag |= CREAD | CLOCAL;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // modo raw
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    // Timeout: 200-500ms conforme especificação
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 5; // 500ms timeout

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

void send_uart(int fd, const uint8_t *buffer, int len) {
    int written = write(fd, buffer, len);
    if (written < 0) {
        perror("Erro ao escrever na UART");
    }
}

int receive_uart(int fd, uint8_t *buffer, int max_len) {
    return read(fd, buffer, max_len);
}

void close_uart(int fd) {
    close(fd);
}
