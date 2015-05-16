# Makefile

CC=
FLAGS=-Wall -O2 
#FLAGS+= -lpthread
OBJ= v4l2_api.o \
	main.o \
	sock_api.o

default: $(OBJ)
	$(CC) $(OBJ) $(FLAGS)

.PHONY:
clean:
	-rm $(OBJ) a.out -rf