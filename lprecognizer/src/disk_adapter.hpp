#pragma once

#include <string>
#include "cJSON.h"
#include "save_adapter.hpp"

namespace IZ {

class DiskAdapter_Config
{
public:
    std::string path = "";
    unsigned int min_size_free_space = 100; //minimal free space on disk (Mb)
    unsigned int max_event_number = 1000;
    unsigned int removal_period_minutes = 10;
};

class DiskAdapter :public SaveAdapter
{
public:
    DiskAdapter(const DiskAdapter_Config & conf)
        :cfg(conf){}

    ~DiskAdapter() {}
    void SaveEvent(std::vector<EventMotionDetection> & event);
    void SaveEvent(std::vector<EventObjectDetection> & event);
    void SaveEvent(std::vector<EventObjectRecognize> & event);

    static void ParseConfig(cJSON* json_sub, DiskAdapter_Config & cfg);
    DiskAdapter_Config cfg;

private:
    void MotionDetectorJsonGenerator(const EventObjectDetection & e, cJSON *jsonItem, const std::string & eventDir, bool saveImages);
    void ObjectsDetectorJsonGenerator(const EventObjectDetection & e, cJSON *detectedObjects, const std::string & eventDir, bool saveImages);
    void ObjectDetectorJsonGenerator(const ResultDetection & r, const std::string & imgFilename, cJSON *jsonObject);
    void ObjectsRecognizerJsonGenerator(const EventObjectRecognize & e, cJSON *detectedObjects, const std::string & eventDir);
    template <class T> void MakeEventFolder(const std::vector<T> & event, std::string & eventDir);
};

}
