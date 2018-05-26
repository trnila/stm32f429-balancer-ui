BIN=frontend_ctrlc
LDFLAGS=-pthread
CCFLAGS=-g -Wall #-fsanitize=address


all: $(BIN)

$(BIN): main.o codec.o serial.o api.o
	$(CC) $^ $(CCFLAGS) $(LDFLAGS) -o $@


install: all
	install -D -m 755 -t /usr/local/bin/ frontend_ctrlc
	install -D -m 755 -t /etc/systemd/system/ frontend_ctrlc.service
	systemctl enable --now frontend_ctrlc.service

uninstall: 
	systemctl disable --now frontend_ctrlc.service
	rm -f /usr/local/bin/frontend_ctrlc /etc/systemd/system/frontend_ctrlc.service

watch:
	journalctl -f -u frontend_ctrlc

clean:
	rm -f *.o $(BIN)
