#pragma once

#include <memory>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

#include "cJSON.h"

namespace IZ {


class EventBase
{
    int64_t timestamp;

    virtual void SaveImages(const std::string & eventDir) const = 0;
    virtual void GenerateJson(cJSON *jsonItem) const = 0;
};

class EventMotionDetection :public EventBase
{
public:
    virtual ~EventMotionDetection(){}
    cv::Mat frame;

    void SaveImages(const std::string & eventDir) const;
    void GenerateJson(cJSON *jsonItem) const;
    std::string FileNameGenerator() const;
};

class MotionDetector
{
public:
    virtual ~MotionDetector(){}
    virtual void Detection(const cv::Mat & input,
                           int64_t timestamp,
                           std::vector<EventMotionDetection> & result) = 0;
};
}
