#include "motion_detector.hpp"

using namespace IZ;

std::string EventMotionDetection::FileNameGenerator() const
{
    return std::to_string(e.timestamp) + ".jpg";
}

//-------------------------------------------------------------------------------------
void EventMotionDetection::SaveImages(const std::string & eventDir) const
{
    std::string imgFilename = eventDir + "/" + FileNameGenerator();

    if(saveImages) {
        if( !cv::imwrite(imgFilename, e.frame) ) {
            throw std::runtime_error("Can't write file: " + imgFilename);
        }
    }
}

//-------------------------------------------------------------------------------------
void EventMotionDetection::GenerateJson(cJSON *jsonItem) const
{
    std::string imgFilename = FileNameGenerator();

    if( cJSON_AddNumberToObject(jsonItem, "timestamp", e.timestamp) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    if( cJSON_AddStringToObject(jsonItem, "frame", imgFilename.c_str()) == NULL) {
        throw std::runtime_error(errorJsonText);
    }
}
