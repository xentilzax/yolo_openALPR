#pragma once

#include <string>
#include "open_alpr_module.hpp"
#include "yolo_detector.hpp"
#include "move_detector.hpp"
#include "simple_motion.hpp"
#include "disk_adapter.hpp"
#include "socket_adapter.hpp"
namespace IZ
{

//---------------------------------------------------------------------------------------------------------------
class Conveer
{
public:
    std::string motionDetectorName;
    std::vector<std::string> motionDetectorAdapters;
    std::string detectorName;
    std::vector<std::string> detectorAdapters;
    std::string recognizerName;
    std::vector<std::string> recognizerAdapters;
};

//---------------------------------------------------------------------------------------------------------------
class Config :
        public ALPR_Config,
        public YOLO_Config,
        public MoveDetector_Config,
        public SimpleMotion_Config,
        public DiskAdapter_Config,
        public SocketAdapter_Config
{
public:
    Conveer conveer;

    int use_yolo_detector = 1;
    std::string camera_url = "";
    std::string camera_id = "A";
    std::string camera_type = "default";
    int gui_enable = 0;
    int verbose_level = 0;
};

//---------------------------------------------------------------------------------------------------------------
extern Config cfg;
}


