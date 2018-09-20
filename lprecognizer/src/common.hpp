#pragma once

#include <string>

namespace Inex
{

//---------------------------------------------------------------------------------------------------------------
typedef struct {
    std::string imagePath;
    std::string textPath;
    unsigned long long timeLabel;
} Event;

//---------------------------------------------------------------------------------------------------------------
class Config
{
public:
    int use_yolo_detector = 1;
    std::string yolo_cfg = "cfg/ishta_sp5.cfg";
    std::string yolo_weights = "weights/ishta_sp5.weights";
    std::string open_alpr_cfg = "cfg/open_alpr.conf";
    std::string open_alpr_skip_cfg = "cfg/open_alpr.conf";
    std::string open_alpr_contry = "us";
    std::string open_alpr_region = "md";
    std::string server = "http://jsonplaceholder.typicode.com/posts";
    std::string path = "";
    std::string camera_url = "";
    std::string camera_id = "A";
    std::string camera_type = "default";
    int gui_enable = 0;
    int verbose_level = 0;
    float yolo_thresh;

    unsigned int min_size_free_space = 100; //minimal free space on disk (Mb)
    unsigned int max_event_number = 1000;
    unsigned int removal_period_minutes = 10;
};

//---------------------------------------------------------------------------------------------------------------
extern Config cfg;
}


