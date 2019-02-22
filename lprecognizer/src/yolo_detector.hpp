#pragma once

#include <memory>

#include "cJSON.h"
#include "object_detector.hpp"
#include "yolo_v2_class.hpp"

namespace IZ {
class YOLO_Config
{
public:
    std::string yolo_cfg = "cfg/ishta_sp5.cfg";
    std::string yolo_weights = "weights/ishta_sp5.weights";
    float yolo_thresh;
};

class YOLO_Detector :public ObjectDetector
{
public:
    YOLO_Detector(const YOLO_Config & cfg);
    YOLO_Detector(cJSON* json);
    ~YOLO_Detector(){}

    void Init();
    static void ParseConfig(cJSON* json, YOLO_Config & cfg);
    void Detection(std::shared_ptr<Event> & event);

    YOLO_Config cfg;
    std::shared_ptr<Detector> yolo;
};
}
