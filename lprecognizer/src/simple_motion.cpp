#include "simple_motion.hpp"

using namespace IZ;


//-------------------------------------------------------------------------------------
void SimpleMotion::ParseConfig(cJSON* json_sub, SimpleMotion_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "skip_frames");
    if (cJSON_IsNumber(json_item)) {
        cfg.simple_skip_frames = json_item->valueint;
    }
}

//-------------------------------------------------------------------------------------
SimpleMotion::SimpleMotion(const SimpleMotion_Config & conf)
    :cfg(conf)
{
}

//-------------------------------------------------------------------------------------
SimpleMotion::SimpleMotion(cJSON* json)
{
    ParseConfig(json, cfg);
}


//-------------------------------------------------------------------------------------
void SimpleMotion::Detection(const cv::Mat & input,
                             int64_t timestamp,
                             std::vector<EventMotionDetection> & result)
{
    EventMotionDetection event;
    event.frame = input;
    event.timestamp = timestamp;
    result.push_back(event);
}
