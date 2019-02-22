#include "object_recognizer.hpp"

using namespace IZ;

//-------------------------------------------------------------------------------------
void EventObjectRecognize::SaveImages(const std::string & eventDir)
{
    EventObjectDetection::SaveImages(eventDir);
}

//-------------------------------------------------------------------------------------
cJSON * EventObjectRecognize::GenerateJson() const
{
    return EventObjectDetection::GenerateJson();
}

//-------------------------------------------------------------------------------------
void DataRecognition::SaveImages(const std::string & eventDir)
{
    DataDetection::SaveImages(eventDir);
}

//-------------------------------------------------------------------------------------
cJSON * DataRecognition::GenerateJson() const
{
    cJSON *jsonObject = DataDetection::GenerateJson();

    if( cJSON_AddBoolToObject(jsonObject, "recognazed", recognized) == NULL) {
        throw std::runtime_error(ERROR_JSON_TEXT);
    }

    if(recognized) {

        cJSON * jsonPlate = cJSON_CreateObject();
        cJSON_AddItemToObject(jsonObject, "best_plate", jsonPlate);

        if( cJSON_AddNumberToObject(jsonPlate, "confidence", bestPlate.confidence) == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        if( cJSON_AddStringToObject(jsonPlate, "characters", bestPlate.characters.c_str()) == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        if( cJSON_AddBoolToObject(jsonPlate, "matches_template", bestPlate.matchesTemplate) == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }

        cJSON* jsonShape = cJSON_CreateArray();
        if (jsonShape == NULL) {
            throw std::runtime_error(ERROR_JSON_TEXT);
        }
        cJSON_AddItemToObject(jsonObject, "topNplates", jsonShape);

        //add top N plates to json
        for(const Plate & p : topNPlates) {
            cJSON* jsonPlate = cJSON_CreateObject();
            if (jsonPlate == NULL) {
                throw std::runtime_error(ERROR_JSON_TEXT);
            }

            if( cJSON_AddNumberToObject(jsonPlate, "confidence", p.confidence) == NULL) {
                throw std::runtime_error(ERROR_JSON_TEXT);
            }

            if( cJSON_AddStringToObject(jsonPlate, "characters", p.characters.c_str()) == NULL) {
                throw std::runtime_error(ERROR_JSON_TEXT);
            }

            if( cJSON_AddBoolToObject(jsonPlate, "matches_template", p.matchesTemplate) == NULL) {
                throw std::runtime_error(ERROR_JSON_TEXT);
            }

            cJSON_AddItemToArray(jsonShape, jsonPlate);
        }
    }
    return jsonObject;
}
