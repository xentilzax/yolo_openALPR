#include "object_recognizer.hpp"

using namespace IZ;

//-------------------------------------------------------------------------------------
void EventObjectRecognize::SaveImages(const std::string & eventDir)
{
    EventObjectDetection::SaveImages(eventDir);
}

//-------------------------------------------------------------------------------------
void EventObjectRecognize::GenerateJson(cJSON * jsonItem) const
{
    //add detected objects to json

    if(recognizedObjects.size() !=  detectedObjects.size() )
        throw std::runtime_error("Size array detected objects and array recognized objects different");

    for(size_t k = 0; k < recognizedObjects.size(); k++) {

        cJSON* jsonObject = cJSON_CreateObject();
        if(jsonObject == NULL) {
            throw std::runtime_error(errorJsonText);
        }
        cJSON_AddItemToArray( jsonItem, jsonObject);

        std::string imgFilename =  FileNameGenerator(k);

        detectedObjects[k].GenerateJson(jsonObject, imgFilename);
        recognizedObjects[k].GenerateJson(jsonObject);
    }
}

//-------------------------------------------------------------------------------------
void ResultRecognition::GenerateJson(cJSON *jsonObject)
{
    if( cJSON_AddBoolToObject(jsonObject, "recognazed", recognized) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    if(r.recognized) {

        cJSON * jsonPlate = cJSON_CreateObject();
        cJSON_AddItemToObject(jsonObject, "best_plate", jsonPlate);

        if( cJSON_AddNumberToObject(jsonPlate, "confidence", bestPlate.confidence) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        if( cJSON_AddStringToObject(jsonPlate, "characters", bestPlate.characters.c_str()) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        if( cJSON_AddBoolToObject(jsonPlate, "matches_template", bestPlate.matchesTemplate) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        cJSON* jsonShape = cJSON_CreateArray();
        if (jsonShape == NULL) {
            throw std::runtime_error(errorJsonText);
        }
        cJSON_AddItemToObject(jsonObject, "topNplates", jsonShape);

        //add top N plates to json
        for(const Plate & p : topNPlates) {
            cJSON* jsonPlate = cJSON_CreateObject();
            if (jsonPlate == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            if( cJSON_AddNumberToObject(jsonPlate, "confidence", p.confidence) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            if( cJSON_AddStringToObject(jsonPlate, "characters", p.characters.c_str()) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            if( cJSON_AddBoolToObject(jsonPlate, "matches_template", p.matchesTemplate) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            cJSON_AddItemToArray(jsonShape, jsonPlate);
        }
    }
}
