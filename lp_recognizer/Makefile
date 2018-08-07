
CPP=g++
SOURCES=src/main.cpp src/post.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=lpr

OPTS=-Ofast
LDFLAGS= -lm -pthread -L./ -l:darknet.so -l:libopenalpr.so
COMMON= -std=c++11
CFLAGS= -Wall -Wfatal-errors -Wno-unused-result -Wno-unknown-pragmas 
CFLAGS+=$(OPTS) 

#cURL
COMMON+= `pkg-config --cflags libcurl`
LDFLAGS+= `pkg-config --libs libcurl`

#OpenCV
CFLAGS+= -DOPENCV
LDFLAGS+= `pkg-config --libs opencv` 
COMMON+= `pkg-config --cflags opencv` 

DEPS = $(wildcard src/*.hpp) Makefile

CFLAGS+= -fPIC
LDFLAGS+= -Wl,-rpath='$$ORIGIN'
INCLUDES=-I /usr/include -I ../darknet/src -I ./src

all: $(SOURCES) $(EXECUTABLE)
	 
$(EXECUTABLE): $(OBJECTS) 
	$(CPP) $(OBJECTS) -o $@ $(COMMON) $(LDFLAGS)

src/post.o: src/post.cpp 
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/post.cpp -o $@ 

src/main.o: src/main.cpp
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/main.cpp -o $@ 

clean:
	rm -rf src/*.o post