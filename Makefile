CC = g++

CFLAGS = -Wall -std=c++0x
DEPS =message_queue.h socket_layer.h pastry_api.h pastry_overlay.h

OBJ = message_queue.o socket_layer.o clienttest.o pastry_api.o pastry_overlay.o

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

main: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto -pthread

clean :
	-rm *.o $(objects) main
