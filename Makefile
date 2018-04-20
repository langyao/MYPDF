SOURCE = $(wildcard src/*.c) $(wildcard utils/*.c) $(wildcard src/*.cpp) $(wildcard utils/*.cpp)  
TARGET = main


CC      = gcc
CFLAGS  = -g -Wall -lz 
#CFLAGS += -DDEBUG
CFLAGS += -Iinc -Iutils 
CFLAGS += -Wno-unused-value  -Wno-unused-variable 

$(TARGET):$(SOURCE)
	$(CC) -o   $@ $^ $(CFLAGS)


clean:
	-rm -f $(TARGET)
