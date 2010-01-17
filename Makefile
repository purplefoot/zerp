GLKINCLUDEDIR = ./glkterm
GLKLIBDIR = $(GLKINCLUDEDIR)

LINKLIBS = -lncurses -lglkterm

OPTIONS = -g

CFLAGS = $(OPTIONS) -I$(GLKINCLUDEDIR)

LIBS = -L$(GLKLIBDIR) $(GLKLIB) $(LINKLIBS) 

HEADERS = glkstart.h zerp.h opcodes.h variables.h zscii.h

SOURCE = glkstart.c main.c zerp.c variables.c zscii.c

OBJS = glkstart.o main.o zerp.o variables.o zscii.o

all: zerp

zerp: $(OBJS)
	$(CC) $(OPTIONS) -o zerp $(OBJS) $(LIBS)

clean:
	rm -f *~ *.o zerp

$(OBJS): $(HEADERS)
