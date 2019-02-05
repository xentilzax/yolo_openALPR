#pragma once

#include <string>
#include "cJSON.h"
#include "save_adapter.hpp"

namespace IZ {

class SocketAdapter_Config
{
public:
    std::string url = "";
};

class SocketAdapter :public SaveAdapter
{
public:
    SocketAdapter(const SocketAdapter_Config & conf)
        :cfg(conf) {}
    ~SocketAdapter() {}
    void SaveEvent(std::vector<EventMotionDetection> & event);
    void SaveEvent(std::vector<EventObjectDetection> & event);
    void SaveEvent(std::vector<EventObjectRecognize> & event);

    static void ParseConfig(cJSON* json_sub, SocketAdapter_Config & cfg);
    SocketAdapter_Config cfg;
};

}
