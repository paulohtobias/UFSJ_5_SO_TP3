#Main Makefile
CC = gcc
CFLAGS = -g -Wall -MMD

#Binary
BIN = main.out

#Directories
IDIR = ./include
SDIR = ./src
ODIR = ./obj

#Files
SOURCE = .c

#Paths
INCLUDE_PATHS = -I$(IDIR)

#Libraries
LIBS = 
#CFLAGS+= `pkg-config --cflags $(LIBS)`
#LIBRARIES = `pkg-config --libs $(LIBS)`

#Compilation line
COMPILE = $(CC) $(CFLAGS) $(INCLUDE_PATHS)

#FILEs
#---------------Source----------------#
SRCS = $(wildcard $(SDIR)/*$(SOURCE)) $(wildcard $(SDIR)/*/*$(SOURCE))

#---------------Object----------------#
OBJS = $(SRCS:$(SDIR)/%$(SOURCE)=$(ODIR)/%.o)
#-------------Dependency--------------#
DEPS = $(SRCS:$(SDIR)/%$(SOURCE)=$(ODIR)/%.d)

all: $(OBJS)
	$(COMPILE) $(OBJS) main$(SOURCE) -o $(BIN) $(LIBRARIES)

dll: LIBRARIES+= -lm -fPIC
dll: $(OBJS)
	$(COMPILE) -shared -o libguisdl.so $(OBJS) $(LIBRARIES)

# Include all .d files
-include $(DEPS)

$(ODIR)/%.o: $(SDIR)/%$(SOURCE)
	$(COMPILE) -c $< -o $@ $(LIBRARIES)

.PHONY : clean
clean :
	-rm $(BIN) *.d $(ODIR)/*

init:
	mkdir -p include
	mkdir -p src
	mkdir -p obj

run:
	./$(BIN)