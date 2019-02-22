#pragma once

#include <opencv2/opencv.hpp>
#include "cJSON.h"
#include "motion_detector.hpp"


namespace IZ {

class SimpleMotion_Config
{
public:
    unsigned int simple_skip_frames = 0;
};

class SimpleMotion :public MotionDetector
{
public:
    SimpleMotion(const SimpleMotion_Config & conf);
    SimpleMotion(cJSON* json);
    ~SimpleMotion(){}

    static void ParseConfig(cJSON* json_sub, SimpleMotion_Config & cfg);
    std::shared_ptr<IZ::Event> Detection(const cv::Mat & input, int64_t timestamp);

    SimpleMotion_Config cfg;
    unsigned int skipedFrames = 0;
};

}
