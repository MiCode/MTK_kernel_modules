#---------------------------------------------------------------------------
TARGET = uart_launcher

OBJECT = \
		 uart_launcher.o \
		 $(NULL)

CFLAGS = \
		 -g \
		 -Wall \
		 -Werror \
		 $(NULL)

#---------------------------------------------------------------------------
ifeq ($(CROSS_COMPILE),)
    CC = gcc
    STRIP = strip
else
    CC = $(CROSS_COMPILE)-gcc
    STRIP = $(CROSS_COMPILE)-strip
endif

#---------------------------------------------------------------------------
all: uart_launcher

uart_launcher: clean $(OBJECT)
	$(CC) -pthread -C -o $(TARGET) $(OBJECT)
	$(STRIP) $(TARGET)

%.o: %.c
	$(CC) $(LINKFLAGS) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f $(TARGET) *.o

#---------------------------------------------------------------------------
