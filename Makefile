CC = gcc
CFLAGS = -Wall -Wextra -std=c11
OBJDIR = bin
TARGET = $(OBJDIR)/jit
OBJS = $(OBJDIR)/main.o

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/main.o: main.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c main.c -o $(OBJDIR)/main.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -rf $(OBJDIR)
