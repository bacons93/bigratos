CC = gcc
CFLAGS = -Wall -Wextra -pedantic

C_SRC = c-src
BINARIES = rat-version rat-info-c rat-search-c rat-add-c

all: $(BINARIES)

rat-version: $(C_SRC)/rat-version.c
	$(CC) $(CFLAGS) $< -o $@

rat-info-c: $(C_SRC)/rat-info.c $(C_SRC)/util.c $(C_SRC)/util.h
	$(CC) $(CFLAGS) $(C_SRC)/rat-info.c $(C_SRC)/util.c -o $@

rat-search-c: $(C_SRC)/rat-search.c $(C_SRC)/util.c $(C_SRC)/util.h
	$(CC) $(CFLAGS) $(C_SRC)/rat-search.c $(C_SRC)/util.c -o $@

rat-add-c: $(C_SRC)/rat-add.c $(C_SRC)/util.c $(C_SRC)/util.h
	$(CC) $(CFLAGS) $(C_SRC)/rat-add.c $(C_SRC)/util.c -o $@

test: all
	./rat-version --help
	./rat-version --version
	./rat-info-c --help
	./rat-info-c --version
	./rat-info-c testpkg || true
	./rat-info-c ../bad || true
	./rat-info-c bad/name || true
	./rat-info-c "$$(printf '%260s' | tr ' ' a)" || true
	./rat-search-c --help
	./rat-search-c --version
	./rat-search-c definitelynotapackage || true
	./rat-search-c ../bad || true
	./rat-search-c bad/name || true
	./rat-search-c "$$(printf '%260s' | tr ' ' a)" || true

clean:
	rm -f $(BINARIES)
