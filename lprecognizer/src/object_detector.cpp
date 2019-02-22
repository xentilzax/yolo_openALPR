#include "object_detector.hpp"

using namespace IZ;


//-------------------------------------------------------------------------------------
void EventObjectDetection::SaveImages(const std::string & eventDir)
{
    EventMotionDetection::SaveImages(eventDir);
}

//-------------------------------------------------------------------------------------
cJSON * EventObjectDetection::GenerateJson() const
{
    return EventMotionDetection::GenerateJson();
}

//-------------------------------------------------------------------------------------
cJSON * ResultDetection::GenerateJson() const
{
    cJSON * jsonObject = ResultMotion::GenerateJson();

    cJSON * jsonArray = cJSON_CreateArray();
    cJSON_AddItemToObject(jsonObject, "detected_objects", jsonArray);

    for(std::shared_ptr<DataObject> r: objectData) {
        cJSON * jsonItem = r->GenerateJson();
        cJSON_AddItemToArray(jsonArray, jsonItem);
    }

    return jsonObject;
}

//-------------------------------------------------------------------------------------
std::string DataDetection::FileNameGenerator() const
{
    return fileName;
}

//-------------------------------------------------------------------------------------
cJSON * DataDetection::GenerateJson() const
{
    cJSON * jsonObject = cJSON_CreateObject();
    if( cJSON_AddNumberToObject(jsonObject, "confidence_detection", confdenceDetection) == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    if( cJSON_AddStringToObject(jsonObject, "cropped_frame", FileNameGenerator().c_str()) == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    cJSON* jsonShape = cJSON_CreateArray();
    if (jsonShape == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    cJSON_AddItemToObject(jsonObject, "plate_boundary", jsonShape);

    //add border of object to json
    for(const cv::Point & p : border.points) {
        cJSON* jsonPoint = cJSON_CreateObject();
        if (jsonPoint == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        if( cJSON_AddNumberToObject(jsonPoint, "x", p.x) == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        if( cJSON_AddNumberToObject(jsonPoint, "y", p.y) == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        cJSON_AddItemToArray(jsonShape, jsonPoint);
    }

    return jsonObject;
}

//-------------------------------------------------------------------------------------
void DataDetection::SaveImages(const std::string & eventDir)
{
    if(!dataAllReadySaved) {
        std::string imgFilename = eventDir + "/" +  FileNameGenerator();

        if( !cv::imwrite(imgFilename, croppedFrame) ) {
            throw std::runtime_error("Can't write file: " + imgFilename);
        }
    }
    dataAllReadySaved = true;
}

//-------------------------------------------------------------------------------------
void ResultDetection::SaveImages(const std::string & eventDir)
{
    ResultMotion::SaveImages(eventDir);

    for(std::shared_ptr<DataObject> obj : objectData) {
        obj->SaveImages(eventDir);
    }
}
