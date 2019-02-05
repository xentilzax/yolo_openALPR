#pragma once

#include <vector>

#include "motion_detector.hpp"
#include "object_detector.hpp"
#include "object_recognizer.hpp"

namespace IZ {

class SaveAdapter
{
public:
    virtual ~SaveAdapter(){}
    virtual void SaveEvent(std::vector<EventMotionDetection> & event) = 0;
    virtual void SaveEvent(std::vector<EventObjectDetection> & event) = 0;
    virtual void SaveEvent(std::vector<EventObjectRecognize> & event) = 0;
};
}
