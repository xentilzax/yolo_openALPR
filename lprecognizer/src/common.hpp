#pragma once

#include <string>

//---------------------------------------------------------------------------------------------------------------
class Config
{
public:
    std::string yolo_cfg = "cfg/ishta_sp5.cfg";
    std::string yolo_weights = "weights/ishta_sp5.weights";
    std::string open_alpr_cfg = "cfg/open_alpr.conf";
    std::string open_alpr_contry = "us";
    std::string open_alpr_region = "md";
    std::string server = "http://jsonplaceholder.typicode.com/posts";
    std::string camera = "";
    int gui_enable = 0;
    int verbose_level = 0;
    float yolo_thresh;
};

extern Config cfg;


