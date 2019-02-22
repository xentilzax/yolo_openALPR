#pragma once

#include <memory>
#include <vector>
#include <string>

#include <opencv2/opencv.hpp>

#include "cJSON.h"

namespace IZ {

#define ERROR_JSON_TEXT "Can't create cJSON item"

//-------------------------------------------------------------------------------------
class EventSerializer
{
public:
    EventSerializer()
        : dataAllReadySaved(false) {}
    virtual ~EventSerializer() {}
    virtual void SaveImages(const std::string & path) = 0;
    virtual cJSON * GenerateJson() const = 0;

protected:
    bool dataAllReadySaved;
};

//-------------------------------------------------------------------------------------
class Result :public EventSerializer
{
public:
    virtual ~Result() {}
    virtual bool IsEmpty() const = 0;
};

//-------------------------------------------------------------------------------------
class Event :public EventSerializer
{
public:
    virtual ~Event() {}
    virtual int64_t GetTimestamp() const = 0;

    std::vector<std::shared_ptr<Result> > arrayResults;
};

//-------------------------------------------------------------------------------------
class ResultMotion :public Result
{
public:
    int64_t timestamp;
    cv::Mat frame;

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;
    bool IsEmpty() const {return false;}
    std::string FileNameGenerator() const;
};

//-------------------------------------------------------------------------------------
class EventMotionDetection :public Event
{
public:
    EventMotionDetection()
        :Event() {}
    EventMotionDetection(const Event* e)
        :Event(*e) {}

    virtual ~EventMotionDetection() {}

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;
    int64_t GetTimestamp() const;
};

//-------------------------------------------------------------------------------------
class MotionDetector
{
public:
    virtual ~MotionDetector() {}
    virtual std::shared_ptr<IZ::Event> Detection(const cv::Mat & input, int64_t timestamp) = 0;
};
}
