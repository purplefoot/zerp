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

HEADERS = glkstart.h zerp.h opcodes.h variables.h zscii.h stack.h debug.h objects.h parse.h

SOURCE = glkstart.c main.c zerp.c opcodes.c variables.c zscii.c stack.c debug.c objects.c parse.c

OBJS = glkstart.o main.o zerp.o opcodes.o variables.o zscii.o stack.o debug.o objects.o parse.o

all: czerp
	mv czerp zerp

zerp: $(OBJS)
	$(CC) $(OPTIONS)  -I./glkterm -o zerp $(OBJS) $(LIBS)

czerp: $(OBJS)
	$(CC) $(OPTIONS) $(CGLKINCLUDE) -o czerp $(OBJS) $(CLIBS)

clean:
	rm -f *~ *.o zerp czerp test/*.z* test/czerp

$(OBJS): $(HEADERS)

test: test/unittests.z3 test_int

ftest:
	frotz test/unittests.z3
	
test/unittests.z3: test_int test/unittests.inf
	inform -v3 -t test/unittests.inf test/unittests.z3
	test/czerp test/unittests.z3

test_int: czerp
	mv czerp test/
