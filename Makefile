TARGET		= ppdemo
OBJS_TARGET	= main.o

CFLAGS += -fopenmp -g
LIBS += -lm -lc -fopenmp

include Makefile.in
