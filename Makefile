# Makefile

FLAGS=-Wall -O2
OBJ= v4l2_api.o \
	main.o \
	sock_api.o

default: $(OBJ)
	$(CC) $(OBJ) $(FLAGS)

.PHONY:
clean:
	-rm $(OBJ) a.out -rf
