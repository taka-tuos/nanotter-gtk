TARGET		= nanotter
OBJS_TARGET	= $(TARGET).o api.o timeline.o gtk.o curl.o misc.o

CFLAGS = -O0 -g -fpermissive `pkg-config --cflags gtk+-3.0`
LDFLAGS = 
LIBS = -ltwitcurl -lncursesw -lcurl -lstdc++ -ljson -lpthread -lc -lm `pkg-config --libs gtk+-3.0`

include Makefile.in
