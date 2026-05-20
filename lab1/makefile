TARGET = myshell
SRCS = myshell.c utility.c
CC = gcc
CFLAGS = -Wall

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

fclean: clean

re: fclean all