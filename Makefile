CC = gcc
CFLAGS = -Wall -Wextra -pedantic

C_SRC = c-src
BINARIES = rat-version rat-info-c

all: $(BINARIES)

rat-version: $(C_SRC)/rat-version.c
	$(CC) $(CFLAGS) $< -o $@

rat-info-c: $(C_SRC)/rat-info.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(BINARIES)
