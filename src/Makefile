INC_DIR= ../include
LIB_DIR= ../lib
BIN_DIR= ../bin

LDFLAG=-lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -pthread -lgthread-2.0 -pthread -lglib-2.0 -lxml2 -L/usr/X11R6/lib -L/usr/local/lib -L$(LIB_DIR)
LIBS= -lARgsub_lite -lARvideo -lAR -lpthread -lglut -lGLU -lGL -lXi -lX11 -lm
CFLAG= -O -pthread -I/usr/include/gstreamer-0.10 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/libxml2 -I/usr/X11R6/include -g -I$(INC_DIR)

OBJS =
HEADERS =

all: $(BIN_DIR)/main

$(BIN_DIR)/main: main.o $(OBJS)
	cc -o $(BIN_DIR)/main main.o $(OBJS) $(LDFLAG) $(LIBS)

main.o: main.c $(HEADERS)
	cc -c $(CFLAG) main.c

clean:
	rm -f *.o
	rm -f $(BIN_DIR)/main

allclean:
	rm -f *.o
	rm -f $(BIN_DIR)/main
	rm -f Makefile
