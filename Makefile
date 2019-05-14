all: conf-mqtt

#install :
#	cp ./client ${TARGET_DIR}/bin/

CC=gcc
CC_FLAGS = -g -I.
LD_FLAGS = -lmosquitto -lpthread -ljansson -lcurl
EXEC = conf-mqtt
SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJECTS = $(SOURCES:.c=.o)

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LD_FLAGS) -o $(EXEC)

# To obtain object files
%.o: %.c $(HEADERS)
	$(CC) -c $(CC_FLAGS) $< -o $@

# To remove generated files
clean:
	rm -rf $(EXEC) $(OBJECTS)

