# make debug kompiluje wersję wypisującą mnóstwo informacji diagnostycznych
# (jednak najpierw należy usunąć wcześniej skompilowaną wersję
# niediagnostyczną poprzez make clean)

CC=gcc
CFLAGS=-Wall
OBJ=superv.o nodeCode.o err.o const.o

all: superv

debug: CFLAGS+=-DDEBUG
debug: superv

superv: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f $(OBJ) superv
