#pragma once

#include "object_detector.hpp"
#include <string>

namespace IZ{

//-------------------------------------------------------------------------------------
class Plate
{
public:
    float confidence;
    std::string characters;
    bool matchesTemplate;
};

//-------------------------------------------------------------------------------------
class DataRecognition :public DataDetection
{
public:
    DataRecognition(const DataDetection* parent)
        :DataDetection(*parent) {}

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;

    bool recognized;
    /// the best plate is the topNPlate with the highest confidence
    Plate bestPlate;
    /// A list of possible plate number permutations
    std::vector<Plate> topNPlates;
};

//-------------------------------------------------------------------------------------
class EventObjectRecognize :public EventObjectDetection
{
public:

    EventObjectRecognize(const Event* e)
        :EventObjectDetection(e)
    {}

    virtual ~EventObjectRecognize() {}

    void SaveImages(const std::string & eventDir);
    cJSON * GenerateJson() const;
};

//-------------------------------------------------------------------------------------
class LicensePlateRecognizer
{
public:
    virtual ~LicensePlateRecognizer() {}
    virtual void Recognize(std::shared_ptr<Event> & event) = 0;
};

}
