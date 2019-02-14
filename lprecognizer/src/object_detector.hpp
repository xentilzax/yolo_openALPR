#pragma once

#include <memory>
#include "motion_detector.hpp"

namespace IZ{

class Shape
{
public:
    Shape(){}
    Shape(cv::Rect & r) {
        points.resize(4);
        points[0] = cv::Point(r.x, r.y);
        points[1] = cv::Point(r.x + r.width, r.y);
        points[2] = cv::Point(r.x + r.width, r.y + r.height);
        points[3] = cv::Point(r.x, r.y + r.height);
    }

    std::vector<cv::Point> points;
};

class ResultDetection
{
public:
    Shape border;
    float confdenceDetection;
    cv::Mat croppedFrame;

    void GenerateJson(cJSON *jsonObject, const std::string & imgFilename) const;
};

class EventObjectDetection :public EventMotionDetection
{
public:
    EventObjectDetection(const EventMotionDetection & e)
        :EventMotionDetection(e)
    {}
    virtual ~EventObjectDetection() {}

    void SaveImages(const std::string & eventDir) const;
    void GenerateJson(cJSON *jsonItem) const;
    std::string FileNameGenerator(int k) const;

    std::vector<ResultDetection> detectedObjects;
};

class ObjectDetector
{
public:
    virtual ~ObjectDetector(){}
    virtual void Detection(const std::vector<EventMotionDetection> & events, std::vector<EventObjectDetection> & results) = 0;
};

}
