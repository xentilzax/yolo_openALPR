OPENCV=1
OPENCV3=1

DEBUG = 0


CPP=g++
CC=gcc

LPR_SRC=src/lpr.cpp src/cJSON.cpp src/config.cpp src/move_detector.cpp src/open_alpr_module.cpp src/simple_motion.cpp src/socket_adapter.cpp src/disk_adapter.cpp src/yolo_detector.cpp
#LPR_OBJ=$(LPR_SRC:.cpp=.o)
LPR_OBJ=src/lpr.o src/cJSON.o src/config.o
LPR_EXE=lprecognizer

DM_SRC=src/dm.cpp src/cJSON.cpp src/config.cpp
DM_OBJ=$(DM_SRC:.cpp=.o)
DM_EXE=diskmanager

WRAP_ALPR_LIB=inex_alpr.so
MOTION_DETECTOR_LIB=inex_motion.so
SIMPLE_DETECTOR_LIB=inex_simple.so
DISK_ADAPTER_LIB=inex_disk.so
SOCKET_ADAPTER_LIB=inex_socket.so
YOLO_DETECTOR_LIB=inex_yolo.so

COMMON_LDFLAGS= -lm -pthread
LPR_LDFLAGS= $(COMMON_LDFLAGS)

#add OpenALPR module
ALPR_LDFLAGS= -L. -l:$(WRAP_ALPR_LIB) -l:libopenalpr.so 
LPR_LDFLAGS+= $(ALPR_LDFLAGS) 

#add service modules
LPR_LDFLAGS+= -L. -l:$(MOTION_DETECTOR_LIB) -l:$(DISK_ADAPTER_LIB) -l:$(SOCKET_ADAPTER_LIB) -l:$(SIMPLE_DETECTOR_LIB)

#add YOLO module
LPR_LDFLAGS+= -L. -l:$(YOLO_DETECTOR_LIB) -L../darknet -l:nn.so 

OPTS=-Ofast
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


DEPS = $(wildcard src/*.hpp) Makefile

CFLAGS+= -fPIC
LPR_LDFLAGS+= -Wl,-rpath='$$ORIGIN'
INCLUDES=-I /usr/include -I ../darknet/src -I ./src


all:$(YOLO_DETECTOR_LIB) $(SOCKET_ADAPTER_LIB) $(DISK_ADAPTER_LIB) $(MOTION_DETECTOR_LIB) $(SIMPLE_DETECTOR_LIB) $(WRAP_ALPR_LIB) $(LPR_EXE) $(DM_EXE)
# all: $(LPR_EXE) $(DM_EXE)

# $(LIBNAMESO): $(OBJS) src/yolo_v2_class.hpp src/yolo_v2_class.cpp
# 	$(CPP) -shared -std=c++11 -fvisibility=hidden -DYOLODLL_EXPORTS $(COMMON) $(CFLAGS) $(OBJS) src/yolo_v2_class.cpp -o $@ $(LDFLAGS)
$(YOLO_DETECTOR_LIB): src/yolo_detector.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@

$(SOCKET_ADAPTER_LIB): src/socket_adapter.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@

$(DISK_ADAPTER_LIB): src/disk_adapter.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@

$(SIMPLE_DETECTOR_LIB): src/simple_motion.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@
	
$(MOTION_DETECTOR_LIB): src/move_detector.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@

$(WRAP_ALPR_LIB): src/open_alpr_module.o 
	$(CPP) -shared  $(COMMON) $(CFLAGS) $^ -o $@

$(LPR_EXE): $(LPR_OBJ) 
	$(CPP) $(LPR_OBJ) -o $@ $(COMMON) $(CFLAGS) $(CURL_LDFLAGS) $(OCV_LDFLAGS) $(LPR_LDFLAGS)
	
#$(DM_EXE): $(DM_OBJ) 
#	$(CPP) $(DM_OBJ) -o $@ $(COMMON)


src/%.o: src/%.cpp
	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c -o $@ $^
	
# src/dm.o: src/dm.cpp
# 	$(CPP) $(COMMON) $(CFLAGS) $(INCLUDES) -c src/dm.cpp -o $@

clean:
	rm -rf src/*.o $(LPR_EXE) $(DM_EXE)