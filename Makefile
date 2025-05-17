# Numele executabilului
EXEC = client

# Fisiere sursa
SRC = client.c requests.c helpers.c buffer.c parson.c

# Fisiere obiect (se genereaza automat)
OBJ = $(SRC:.c=.o)

# Compilator si optiuni
CC = gcc
CFLAGS = -Wall -g

# Regula implicita: construim executabilul
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Regula pentru curatare
clean:
	rm -f $(OBJ) $(EXEC)
