#pragma once

#include <iostream>
#include <memory>

#include "alpr.h"
#include "cJSON.h"
#include "object_detector.hpp"
#include "object_recognizer.hpp"

namespace IZ {

class ALPR_Config
{
public:
    std::string open_alpr_cfg = "cfg/open_alpr.conf";
    std::string open_alpr_contry = "us";
    std::string open_alpr_region = "md";
    int open_alpr_num_variation = 1;
};

class ALPR_Module :public IZ::ObjectDetector, public IZ::LicensePlateRecognizer
{
public:
    ALPR_Module(const ALPR_Config & cfg);
    ALPR_Module(cJSON* json);
    ~ALPR_Module(){}

    void Init();
    static void ParseConfig(cJSON* json, ALPR_Config & cfg);
    void Detection(const std::vector<IZ::EventMotionDetection> & events, std::vector<IZ::EventObjectDetection> & results);
    void Recognize(std::vector<IZ::EventObjectDetection> & events, std::vector<IZ::EventObjectRecognize> & result);

    ALPR_Config cfg;

    bool open_alpr_use_detector = false;
    static std::shared_ptr<alpr::Alpr> openalpr;
    std::vector<alpr::AlprResults> alprResults;
};
}
