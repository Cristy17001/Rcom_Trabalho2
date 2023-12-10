CC = gcc
SRC = main.c functions.c
OBJ = $(SRC:.c=.o)
EXEC = main

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $< -o $@

clean_all:
	rm -f $(filter-out $(wildcard *.c) $(wildcard *.h) Makefile, $(wildcard *))

clean:
	rm -f $(OBJ) $(EXEC)
	
run: $(EXEC)
	./$(EXEC) $(filter-out $@,$(MAKECMDGOALS))

timestamp: $(EXEC)
	./$(EXEC) ftp://anonymous:anonymous@ftp.up.pt/pub/kodi/timestamp.txt

pipe: $(EXEC)
	./$(EXEC) ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt

%:
	@:
