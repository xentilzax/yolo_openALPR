#pragma once

#include "object_detector.hpp"
#include <string>

namespace IZ{

class Plate
{
public:
    float confidence;
    std::string characters;
    bool matchesTemplate;
};

class ResultRecognition :public ResultDetection
{
public:
    ResultRecognition(const ResultDetection & rd)
        :ResultDetection(rd) {}

    bool recognized;

    /// the best plate is the topNPlate with the highest confidence
    Plate bestPlate;

    /// A list of possible plate number permutations
    std::vector<Plate> topNPlates;
};

class EventObjectRecognize :public EventMotionDetection
{
public:

    EventObjectRecognize(const EventObjectDetection & e)
        :EventMotionDetection(reinterpret_cast<const EventMotionDetection &>(e)) {}

    virtual ~EventObjectRecognize(){}

   std::vector<ResultRecognition> recognizedObjects;
};

class LicensePlateRecognizer
{
public:
    virtual ~LicensePlateRecognizer(){}
    virtual void Recognize(std::vector<EventObjectDetection> & events, std::vector<EventObjectRecognize> & result) = 0;
};

}
