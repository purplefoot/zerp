OPTIONS = -g

GLKDIR = ./glkterm
CGLKDIR = ./cheapglk

CFLAGS = $(OPTIONS) $(GLKINCLUDE)

ifeq ($($@),czerp)
	CFLAGS = $(OPTIONS) $(CGLKINCLUDE)
endif

GLKINCLUDE = -I./glkterm
CGLKINCLUDE = -I$(CGLKDIR)

LIBS = -L$(GLKDIR) -lncurses -lglkterm
CLIBS = -L$(CGLKDIR) -lcheapglk

HEADERS = glkstart.h zerp.h opcodes.h variables.h zscii.h stack.h debug.h

SOURCE = glkstart.c main.c zerp.c variables.c zscii.c stack.c debug.c

OBJS = glkstart.o main.o zerp.o variables.o zscii.o stack.o debug.o

all: czerp

zerp: $(OBJS)
	$(CC) $(OPTIONS)  -I./glkterm -o zerp $(OBJS) $(LIBS)

czerp: $(OBJS)
	$(CC) $(OPTIONS) $(CGLKINCLUDE) -o zerp $(OBJS) $(CLIBS)

clean:
	rm -f *~ *.o zerp

$(OBJS): $(HEADERS)
