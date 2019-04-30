#pragma once

#include <memory>
#include "motion_detector.hpp"

namespace IZ{

//-------------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------------
class DataObject :public EventSerializer
{
public:
    virtual ~DataObject() {}
};

//-------------------------------------------------------------------------------------
class ResultDetection :public ResultMotion
{
public:
    ResultDetection(const ResultMotion *parent)
        : ResultMotion(*parent) {}
    virtual ~ResultDetection() {}

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;
    bool IsEmpty() const
    {
        return objectData.empty();
    }

    std::vector<std::shared_ptr<DataObject> > objectData;
};

//-------------------------------------------------------------------------------------
class DataDetection :public DataObject
{
public:
    DataDetection(const ResultDetection* parent, int index)
        : DataObject()
    {
        fileName = std::to_string(parent->timestamp) + "-" + std::to_string(index) + ".jpg";
    }
    virtual ~DataDetection() {}

    Shape border;
    float confdenceDetection;
    cv::Mat croppedFrame;

    void SaveImages(const std::string & eventDir);
    cJSON* GenerateJson() const;
    std::string FileNameGenerator() const;

private:
    std::string fileName;
};

//-------------------------------------------------------------------------------------
class EventObjectDetection :public EventMotionDetection
{
public:
    EventObjectDetection(const Event* e)
        :EventMotionDetection(e)
    {}

    virtual ~EventObjectDetection() {}

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;
};

//-------------------------------------------------------------------------------------
class ObjectDetector
{
public:
    virtual ~ObjectDetector() {}
    virtual std::shared_ptr<IZ::Event> Detection(std::shared_ptr<IZ::Event> event) = 0;
};

}
