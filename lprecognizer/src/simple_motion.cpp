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
std::shared_ptr<IZ::Event> SimpleMotion::Detection(const cv::Mat & input, int64_t timestamp)
{
    std::shared_ptr<EventMotionDetection> result = std::make_shared<EventMotionDetection>();
    if(skipedFrames < cfg.simple_skip_frames) {
        skipedFrames++;
    } else {
        std::shared_ptr<ResultMotion> rm( new ResultMotion());
        rm->frame = input;
        rm->timestamp = timestamp;
        result->arrayResults.push_back(std::static_pointer_cast<Result>(rm));
        skipedFrames = 0;
    }

    return result;
}
