#include "motion_detector.hpp"

using namespace IZ;

//const std::string ERROR_JSON_TEXT("Can't create cJSON item");

//-------------------------------------------------------------------------------------
void EventMotionDetection::SaveImages(const std::string & eventDir)
{
    for(std::shared_ptr<Result> r: arrayResults) {
        r->SaveImages(eventDir);
    }
}

//-------------------------------------------------------------------------------------
cJSON* EventMotionDetection::GenerateJson() const
{
    cJSON * json = cJSON_CreateArray();
    for(std::shared_ptr<Result> r: arrayResults) {
        cJSON *jsonItem = r->GenerateJson();
        cJSON_AddItemToArray(json, jsonItem);
    }

    return json;
}

int64_t EventMotionDetection::GetTimestamp() const
{
    if(arrayResults.empty()) {
        return 0;
    }

    int64_t minTimestamp = std::dynamic_pointer_cast<ResultMotion>(arrayResults[0])->timestamp;
    for(std::shared_ptr<Result> item: arrayResults) {
        std::shared_ptr<ResultMotion> result = std::dynamic_pointer_cast<ResultMotion>(item);
        if(result->timestamp < minTimestamp) {
            minTimestamp = result->timestamp;
        }
    }
    return minTimestamp;
}
//-------------------------------------------------------------------------------------
std::string ResultMotion::FileNameGenerator() const
{
    return std::to_string(timestamp) + ".jpg";
}

//-------------------------------------------------------------------------------------
void ResultMotion::SaveImages(const std::string & eventDir)
{
    if(!dataAllReadySaved) {
        std::string imgFilename = eventDir + "/" + FileNameGenerator();

        if( !cv::imwrite(imgFilename, frame) ) {
            throw std::runtime_error("Can't write file: " + imgFilename);
        }

    }
    dataAllReadySaved = true;
}

//-------------------------------------------------------------------------------------
cJSON* ResultMotion::GenerateJson() const
{
    cJSON *jsonItem = cJSON_CreateObject();

    if( cJSON_AddNumberToObject(jsonItem, "timestamp", timestamp) == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    if( cJSON_AddStringToObject(jsonItem, "frame", FileNameGenerator().c_str()) == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    return jsonItem;
}
