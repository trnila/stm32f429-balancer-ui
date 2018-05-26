BIN=frontend_ctrl
LDFLAGS=-pthread
CCFLAGS=-g -Wall #-fsanitize=address


all: $(BIN)

$(BIN): main.o codec.o serial.o api.o
	$(CC) $^ $(CCFLAGS) $(LDFLAGS) -o $@


clean:
	rm -f *.o $(BIN)
