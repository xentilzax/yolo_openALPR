OPENCV=1
OPENCV3=1

DEBUG = 0


CPP=g++
CC=gcc

LPR_SRC=src/lpr.cpp src/post.cpp src/cJSON.c src/config.cpp
LPR_OBJ=$(LPR_SRC:.cpp=.o)
LPR_EXE=lprecognizer

DM_SRC=src/dm.cpp src/cJSON.c src/config.cpp
DM_OBJ=$(DM_SRC:.cpp=.o)
DM_EXE=diskmanager

OPTS=-Ofast
COMMON_LDFLAGS= -lm -pthread
LPR_LDFLAGS= $(COMMON_LDFLAGS) -L../darknet -l:nn.so -l:libopenalpr.so

COMMON= -std=c++11
CFLAGS= -Wall -Wfatal-errors -Wno-unused-result -Wno-unknown-pragmas 
CFLAGS+=$(OPTS)

ifeq ($(DEBUG), 1) 
OPTS= -O0 -g
endif

#cURL
#COMMON+= `pkg-config --cflags libcurl`
CURL_LDFLAGS+= `pkg-config --libs libcurl`


#OpenCV
ifeq ($(OPENCV), 1) 
#COMMON+= -DOPENCV
CFLAGS+= -DOPENCV
ifeq ($(OPENCV3), 1) 
OCV_LDFLAGS = \
    -l:libopencv_videoio.so.3.3 \
    -l:libopencv_video.so.3.3 \
    -l:libopencv_highgui.so.3.3 \
    -l:libopencv_imgproc.so.3.3 \
    -l:libopencv_imgcodecs.so.3.3 \
    -l:libopencv_core.so.3.3
else
OCV_LDFLAGS = \
    -l"opencv_video" \
    -l"opencv_highgui" \
    -l"opencv_imgproc" \
    -l"opencv_core"
endif
endif

#LDFLAGS+= `pkg-config --libs opencv` 
#COMMON+= `pkg-config --cflags opencv` 

DEPS = $(wildcard src/*.hpp) Makefile

CFLAGS+= -fPIC
LPR_LDFLAGS+= -Wl,-rpath='$$ORIGIN'
INCLUDES=-I /usr/include -I ../darknet/src -I ./src

all: $(LPR_SRCS) $(LPR_EXE) $(DM_EXE)
	 
$(LPR_EXE): $(LPR_OBJ) 
	$(CPP) $(LPR_OBJ) -o $@ $(COMMON) $(CURL_LDFLAGS) $(OCV_LDFLAGS) $(LPR_LDFLAGS)
	
$(DM_EXE): $(DM_OBJ) 
	$(CPP) $(DM_OBJ) -o $@ $(COMMON) $(COMMON_LDFLAGS)
	
src/cJSON.o: src/cJSON.c 
	$(CC) $(OPTS) -c src/cJSON.c -o $@ 
	
src/post.o: src/post.cpp 
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/post.cpp -o $@

src/config.o: src/config.cpp 
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/config.cpp -o $@ 

src/lpr.o: src/lpr.cpp
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/lpr.cpp -o $@
	
src/dm.o: src/dm.cpp
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/dm.cpp -o $@

clean:
	rm -rf src/*.o $(LPR_EXE) $(DM_EXE)