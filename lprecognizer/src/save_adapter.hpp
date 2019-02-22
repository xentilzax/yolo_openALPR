#pragma once

#include <vector>

#include "motion_detector.hpp"
#include "object_detector.hpp"
#include "object_recognizer.hpp"

namespace IZ {

class SaveAdapter
{
public:
    virtual ~SaveAdapter() {}
    virtual void SaveEvent(std::shared_ptr<Event> event) = 0;
};
}
