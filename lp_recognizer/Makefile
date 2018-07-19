OPENCV=1

OBJDIR=./obj/
APPNAME=demo

CPP=g++
OPTS=-Ofast
LDFLAGS= -lm -pthread 
COMMON= 
CFLAGS=-Wall -Wfatal-errors -Wno-unused-result -Wno-unknown-pragmas
CFLAGS+=$(OPTS)

ifeq ($(OPENCV), 1) 
CFLAGS+= -DOPENCV
LDFLAGS+= `pkg-config --libs opencv` 
COMMON+= `pkg-config --cflags opencv` 
endif


OBJ=main.o 

OBJS = $(addprefix $(OBJDIR), $(OBJ))
DEPS = $(wildcard src/*.h) Makefile

all: obj $(APPNAME)

CFLAGS+= -fPIC
LDFLAGS+= -Wl,-rpath='$$ORIGIN'
	
$(APPNAME): src/main.cpp 
	$(CPP) -std=c++11 $(COMMON) $(CFLAGS) -o $@ src/main.cpp $(LDFLAGS) -I /usr/include -L/usr/lib  -I../darknet/src -L./ -l:darknet.so -l:libopenalpr.so

$(OBJDIR)%.o: %.cpp $(DEPS)
	$(CPP) $(COMMON) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p obj

.PHONY: clean

clean:
	rm -rf $(OBJS) $(APPNAME)

