OPENCV=1
OPENCV3=1

DEBUG = 0


CPP=g++
CC=gcc
SOURCES=src/main.cpp src/post.cpp src/cJSON.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=lprecognizer

OPTS=-Ofast
LDFLAGS= -lm -pthread -L../darknet -l:nn.so -l:libopenalpr.so
COMMON= -std=c++11
CFLAGS= -Wall -Wfatal-errors -Wno-unused-result -Wno-unknown-pragmas 
CFLAGS+=$(OPTS) 

ifeq ($(DEBUG), 1) 
OPTS= -O0 -g
endif

#cURL
COMMON+= `pkg-config --cflags libcurl`
LDFLAGS+= `pkg-config --libs libcurl`


#OpenCV
ifeq ($(OPENCV), 1) 
COMMON+= -DOPENCV
CFLAGS+= -DOPENCV
ifeq ($(OPENCV3), 1) 
LDFLAGS += \
    -l:libopencv_videoio.so.3.3 \
    -l:libopencv_video.so.3.3 \
    -l:libopencv_highgui.so.3.3 \
    -l:libopencv_imgproc.so.3.3 \
    -l:libopencv_imgcodecs.so.3.3 \
    -l:libopencv_core.so.3.3
else
LDFLAGS += \
    -l"opencv_video" \
    -l"opencv_highgui" \
    -l"opencv_imgproc" \
    -l"opencv_core"
endif
endif
#CFLAGS+= -DOPENCV
#LDFLAGS+= `pkg-config --libs opencv` 
#COMMON+= `pkg-config --cflags opencv` 

DEPS = $(wildcard src/*.hpp) Makefile

CFLAGS+= -fPIC
LDFLAGS+= -Wl,-rpath='$$ORIGIN'
INCLUDES=-I /usr/include -I ../darknet/src -I ./src

all: $(SOURCES) $(EXECUTABLE)
	 
$(EXECUTABLE): $(OBJECTS) 
	$(CPP) $(OBJECTS) -o $@ $(COMMON) $(LDFLAGS)
	
src/cJSON.o: src/cJSON.c 
	$(CC) $(OPTS) -c src/cJSON.c -o $@ 
	
src/post.o: src/post.cpp 
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/post.cpp -o $@ 

src/main.o: src/main.cpp
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/main.cpp -o $@ 

clean:
	rm -rf src/*.o $(EXECUTABLE)