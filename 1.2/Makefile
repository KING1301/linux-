CC=gcc
EXEC=num clock cpu
EX=main
default: $(EXEC)
$(EXEC):%:%.c
	$(CC) -o $@ $< `pkg-config --cflags --libs gtk+-2.0`
all:$(EX)
$(EX):%:%.c
	$(CC) -o $@ $< -lpthread
clean:
	-rm  $(EXEC) $(EX)
