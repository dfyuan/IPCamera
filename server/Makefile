OBJ=v4l2.o server.o utils.o tcputils.o webcam.o
CC=arm-linux-gcc
CFLAGS= -DDEBUG
all: cam_server
cam_server: $(OBJ)
	$(CC) $^ -o $@ -lpthread
clean:
	$(RM) *.o cam_server
