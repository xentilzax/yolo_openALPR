#include "object_detector.hpp"

using namespace IZ;


//-------------------------------------------------------------------------------------
std::string EventObjectDetection::FileNameGenerator(int k) const
{
    return std::to_string(e.timestamp) + "-" + std::to_string(k) + ".jpg";
}

//-------------------------------------------------------------------------------------
void EventObjectDetection::SaveImages(const std::string & eventDir) const
{
    EventMotionDetection::SaveImages(eventDir);
    int k = 0;
    for(const ResultDetection & r : e.detectedObjects) {
        std::string imgFilename = eventDir + "/" +  FileNameGenerator(k);

        if( !cv::imwrite(imgFilename, r.croppedFrame) ) {
            throw std::runtime_error("Can't write file: " + imgFilename);
        }
    }
}

//-------------------------------------------------------------------------------------
void EventObjectDetection::GenerateJson(cJSON *jsonItem) const
{
    //add detected objects to json
    int k = 0;
    for(const ResultDetection & r : detectedObjects) {
        std::string imgFilename = FileNameGenerator(k);

        cJSON* jsonObject = cJSON_CreateObject();
        if(jsonObject == NULL) {
            throw std::runtime_error(errorJsonText);
        }
        cJSON_AddItemToArray(jsonItem, jsonObject);

        r.GenerateJson(jsonObject, imgFilename);

        k++;
    }
}

//-------------------------------------------------------------------------------------
void ResultDetection::GenerateJson(cJSON *jsonObject, const std::string & imgFilename) const
{
    if( cJSON_AddNumberToObject(jsonObject, "confidence_detecton", confdenceDetection) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    if( cJSON_AddStringToObject(jsonObject, "cropped_frame", imgFilename.c_str()) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    cJSON* jsonShape = cJSON_CreateArray();
    if (jsonShape == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    cJSON_AddItemToObject(jsonObject, "plate_boundary", jsonShape);

    //add border of object to json
    for(const cv::Point & p : border.points) {
        cJSON* jsonPoint = cJSON_CreateObject();
        if (jsonPoint == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        if( cJSON_AddNumberToObject(jsonPoint, "x", p.x) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        if( cJSON_AddNumberToObject(jsonPoint, "y", p.y) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        cJSON_AddItemToArray(jsonShape, jsonPoint);
    }


}
