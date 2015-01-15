CC = gcc

CFLAGS  = -fno-strict-aliasing -g -O2 -m64 -Wall -Wno-uninitialized -Wno-unknown-pragmas 
LIBS = -lproc -lnsl 
TARGET = ptcpstat

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LIBS)

clean:
	$(RM) $(TARGET)
