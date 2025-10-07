CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = 

# Arquivos objeto
OBJS = crc16.o uart.o modbus_parking.o

# Biblioteca estática
LIB = libmodbus_parking.a

# Exemplo
EXAMPLE = example_parking

all: $(LIB) $(EXAMPLE)

$(LIB): $(OBJS)
	ar rcs $@ $^
	@echo "Biblioteca criada: $(LIB)"

$(EXAMPLE): example_parking.o $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -L. -lmodbus_parking $(LDFLAGS)
	@echo "Exemplo compilado: $(EXAMPLE)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(LIB) $(EXAMPLE)
	@echo "Arquivos limpos"

install: $(LIB)
	@mkdir -p ../lib ../include
	cp $(LIB) ../lib/
	cp *.h ../include/
	@echo "Biblioteca instalada em ../lib e headers em ../include"

.PHONY: all clean install
