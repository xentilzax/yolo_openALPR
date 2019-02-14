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

class ResultRecognition
{
public:
    ResultRecognition() {}

    void GenerateJson(cJSON *jsonItemr) const;

    bool recognized;
    /// the best plate is the topNPlate with the highest confidence
    Plate bestPlate;
    /// A list of possible plate number permutations
    std::vector<Plate> topNPlates;

};

class EventObjectRecognize :public EventObjectDetection
{
public:

    EventObjectRecognize(const EventObjectDetection & e)
        :EventObjectDetection(e)
    {}

    virtual ~EventObjectRecognize() {}

    void SaveImages(const std::string & eventDir) const;
    void GenerateJson(cJSON *jsonItemr) const;

   std::vector<ResultRecognition> recognizedObjects;
};

class LicensePlateRecognizer
{
public:
    virtual ~LicensePlateRecognizer(){}
    virtual void Recognize(std::vector<EventObjectDetection> & events, std::vector<EventObjectRecognize> & result) = 0;
};

}
