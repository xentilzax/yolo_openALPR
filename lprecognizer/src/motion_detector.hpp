#pragma once

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>

namespace IZ {

class EventMotionDetection
{
public:
    virtual ~EventMotionDetection(){}
    cv::Mat frame;
    int64_t timestamp;
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
