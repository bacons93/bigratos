CC = gcc
CFLAGS = -Wall -Wextra -pedantic

C_SRC = c-src
BINARIES = rat-version rat-info-c rat-search-c

all: $(BINARIES)

rat-version: $(C_SRC)/rat-version.c
	$(CC) $(CFLAGS) $< -o $@

rat-info-c: $(C_SRC)/rat-info.c
	$(CC) $(CFLAGS) $< -o $@

rat-search-c: $(C_SRC)/rat-search.c
	$(CC) $(CFLAGS) $< -o $@

test: all
	./rat-version --help
	./rat-version --version
	./rat-info-c --help
	./rat-info-c --version
	./rat-info-c testpkg || true
	./rat-info-c ../bad || true
	./rat-info-c bad/name || true
	./rat-search-c --help
	./rat-search-c --version
	./rat-search-c definitelynotapackage || true

clean:
	rm -f $(BINARIES)
