#include "disk_adapter.hpp"

#include <iostream>
#include <fstream>

using namespace IZ;

//-------------------------------------------------------------------------------------
std::string errorJsonText = "Can't create cJSON item";

//-------------------------------------------------------------------------------------
bool MakeFolder (const std::string & path, int64_t time_id, std::string & event_dir);
bool CheckExitsAndMakeDir(const std::string & dir);

//-------------------------------------------------------------------------------------
void DiskAdapter::ParseConfig(cJSON* json_sub, DiskAdapter_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "path");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.path = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "max_event_number");
    if (cJSON_IsNumber(json_item)) {
        cfg.max_event_number = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "min_size_free_space");
    if (cJSON_IsNumber(json_item)) {
        cfg.min_size_free_space = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "removal_period_minutes");
    if (cJSON_IsNumber(json_item)) {
        cfg.removal_period_minutes = json_item->valueint;
    }
}

//-------------------------------------------------------------------------------------
void DiskAdapter::SaveEvent(std::vector<EventMotionDetection> & event)
{
    if( event.size() == 0)
        return;

    //find minimal timestamp from events
    int64_t minTimestamp = event.begin()->timestamp;
    for(const auto & e: event) {
        if(e.timestamp < minTimestamp) {
            minTimestamp = e.timestamp;
        }
    }

    std::string eventDir;
    if (!MakeFolder (cfg.path, minTimestamp, eventDir)) {
        return;
    }

    //save json data
    cJSON * json = cJSON_CreateArray();

    for(const auto & e: event) {
        cJSON *jsonItem = cJSON_CreateObject();
        cJSON_AddItemToArray(json, jsonItem);

        //motion sector
        MotionDetectorJsonGenerator(e, jsonItem, eventDir, true);
    }

    std::shared_ptr<char> textJson( cJSON_Print(json) );
    cJSON_Delete(json);

    if (textJson.get() == NULL) {
        std::cerr << "Failed to print cJSON\n";
        return;
    }

    std::string textFilename = eventDir + "/data.txt";
    std::ofstream textFile;
    textFile.open(textFilename);
    textFile << std::string(textJson.get());
    textFile.close();
}

//-------------------------------------------------------------------------------------
void DiskAdapter::SaveEvent(std::vector<EventObjectDetection> & event)
{
    if( event.size() == 0)
        return;

    std::string eventDir;
    MakeEventFolder(event, eventDir);

    //save json data
    cJSON * json;
    json = cJSON_CreateArray();

    try{
        for(const auto & e: event) {

            cJSON *jsonItem = cJSON_CreateObject();
            cJSON_AddItemToArray(json, jsonItem);

            //motion sector
            MotionDetectorJsonGenerator(e, jsonItem, eventDir, false);

            //detector sector
            cJSON * detectedObjects = cJSON_CreateArray();
            cJSON_AddItemToObject(jsonItem, "objects", detectedObjects);

            ObjectsDetectorJsonGenerator(e, detectedObjects, eventDir, true);

        }
    } catch (std::runtime_error & e) {
        std::cerr << e.what() << std::endl;

        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            std::cerr << "DiskAdapter: cJSON module: " << std::string(error_ptr) << std::endl;
        }

        cJSON_Delete(json);
        return;
    }


    std::shared_ptr<char> textJson( cJSON_Print(json) );
    cJSON_Delete(json);

    if (textJson.get() == NULL) {
        std::cerr << "Failed to print cJSON\n";
        return;
    }

    std::string textFilename = eventDir + "/data.txt";
    std::ofstream textFile;
    textFile.open(textFilename);
    textFile << std::string(textJson.get());
    textFile.close();
}

//-------------------------------------------------------------------------------------
void DiskAdapter::SaveEvent(std::vector<EventObjectRecognize> & event)
{
    if( event.size() == 0)
        return;

    std::string eventDir;
    MakeEventFolder(event, eventDir);

    //save json data
    cJSON * json;
    json = cJSON_CreateArray();

    try{
        for(const auto & e: event) {

            cJSON *jsonItem = cJSON_CreateObject();
            cJSON_AddItemToArray(json, jsonItem);

            //motion sector
            MotionDetectorJsonGenerator(e, jsonItem, eventDir, false);

            //detector sector
            cJSON * detectedObjects = cJSON_CreateArray();
            cJSON_AddItemToObject(jsonItem, "objects", detectedObjects);

            //            ObjectDetectorJsonGenerator(e, detectedObjects, eventDir, false);

            //            //recognize sector
            //            cJSON * recognizerObjects = cJSON_CreateArray();
            //            cJSON_AddItemToObject(jsonItem, "recognized_objects", recognizerObjects);

            ObjectsRecognizerJsonGenerator(e, detectedObjects, eventDir);

        }
    } catch (std::runtime_error & e) {
        std::cerr << e.what() << std::endl;

        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            std::cerr << "DiskAdapter: cJSON module: " << std::string(error_ptr) << std::endl;
        }

        cJSON_Delete(json);
        return;
    }


    std::shared_ptr<char> textJson( cJSON_Print(json) );
    cJSON_Delete(json);

    if (textJson.get() == NULL) {
        std::cerr << "Failed to print cJSON\n";
        return;
    }

    std::string textFilename = eventDir + "/data.txt";
    std::ofstream textFile;
    textFile.open(textFilename);
    textFile << std::string(textJson.get());
    textFile.close();
}

//-------------------------------------------------------------------------------------
template <class T>
void DiskAdapter::MakeEventFolder(const std::vector<T> & event, std::string & eventDir)
{
    //find minimal timestamp from events
    int64_t minTimestamp = event.begin()->timestamp;
    for(const auto & e: event) {
        if(e.timestamp < minTimestamp) {
            minTimestamp = e.timestamp;
        }
    }

    if (!MakeFolder (cfg.path, minTimestamp, eventDir)) {
        throw std::runtime_error("Can't make folder for timestamp: " + std::to_string(minTimestamp) + " in default path: " + cfg.path);
    }
}

//-------------------------------------------------------------------------------------
void DiskAdapter::MotionDetectorJsonGenerator(const EventObjectDetection & e, cJSON *jsonItem, const std::string & eventDir, bool saveImages)
{
    std::string imgFilename = eventDir + "/" + std::to_string(e.timestamp) + ".jpg";

    if(saveImages) {
        if( !cv::imwrite(imgFilename, e.frame) ) {
            throw std::runtime_error("Can't write file: " + imgFilename);
        }
    }

    if( cJSON_AddNumberToObject(jsonItem, "timestamp", e.timestamp) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    if( cJSON_AddStringToObject(jsonItem, "frame", imgFilename.c_str()) == NULL) {
        throw std::runtime_error(errorJsonText);
    }
}

//-------------------------------------------------------------------------------------
void DiskAdapter::ObjectsDetectorJsonGenerator(const EventObjectDetection & e, cJSON *detectedObjects, const std::string & eventDir, bool saveImages)
{
    //add detected objects to json
    int k = 0;
    for(const ResultDetection & r : e.detectedObjects) {
        std::string imgFilename = eventDir + "/" + std::to_string(e.timestamp) + "_" + std::to_string(k) + ".jpg";

        if(saveImages) {
            if( !cv::imwrite(imgFilename, r.croppedFrame) ) {
                throw std::runtime_error("Can't write file: " + imgFilename);
            }
        }

        cJSON* jsonObject = cJSON_CreateObject();
        if(jsonObject == NULL) {
            throw std::runtime_error(errorJsonText);
        }
        cJSON_AddItemToArray(detectedObjects, jsonObject);

        ObjectDetectorJsonGenerator(r, imgFilename, jsonObject);

        k++;
    }
}
//-------------------------------------------------------------------------------------
void DiskAdapter::ObjectDetectorJsonGenerator(const ResultDetection & r, const std::string & imgFilename, cJSON *jsonObject)
{
    if( cJSON_AddNumberToObject(jsonObject, "confidence_detecton", r.confdenceDetection) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    if( cJSON_AddStringToObject(jsonObject, "cropped_frame", imgFilename.c_str()) == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    cJSON* jsonShape = cJSON_CreateArray();
    if (jsonShape == NULL) {
        throw std::runtime_error(errorJsonText);
    }

    cJSON_AddItemToObject(jsonObject, "shape", jsonShape);

    //add border of object to json
    for(const cv::Point & p : r.border.points) {
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

//-------------------------------------------------------------------------------------
void DiskAdapter::ObjectsRecognizerJsonGenerator(const EventObjectRecognize & e, cJSON *detectedObjects, const std::string & eventDir)
{
    //add detected objects to json
    int k = 0;
    for(const ResultRecognition & r : e.recognizedObjects) {

        cJSON* jsonObject = cJSON_CreateObject();
        if(jsonObject == NULL) {
            throw std::runtime_error(errorJsonText);
        }
        cJSON_AddItemToArray(detectedObjects, jsonObject);

        std::string imgFilename = eventDir + "/" + std::to_string(e.timestamp) + "_" + std::to_string(k) + ".jpg";
        ObjectDetectorJsonGenerator(r, imgFilename, jsonObject);

        if( cJSON_AddBoolToObject(jsonObject, "recognazed", r.recognized) == NULL) {
            throw std::runtime_error(errorJsonText);
        }

        if(r.recognized) {

            cJSON * jsonPlate = cJSON_CreateObject();
            cJSON_AddItemToObject(jsonObject, "best_plate", jsonPlate);

            if( cJSON_AddNumberToObject(jsonPlate, "confidence", r.bestPlate.confidence) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            if( cJSON_AddStringToObject(jsonPlate, "characters", r.bestPlate.characters.c_str()) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            if( cJSON_AddBoolToObject(jsonPlate, "matches_template", r.bestPlate.matchesTemplate) == NULL) {
                throw std::runtime_error(errorJsonText);
            }

            cJSON* jsonShape = cJSON_CreateArray();
            if (jsonShape == NULL) {
                throw std::runtime_error(errorJsonText);
            }
            cJSON_AddItemToObject(jsonObject, "topNplates", jsonShape);

            //add top N plates to json
            for(const Plate & p : r.topNPlates) {
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

        k++;
    }
}

//-------------------------------------------------------------------------------------
bool MakeFolder (const std::string & path, int64_t time_id, std::string & event_dir)
{
    time_t t = static_cast<time_t>(time_id/1000);

    std::tm tm = *std::localtime(&t);

    if ( !CheckExitsAndMakeDir(path)) { std::cerr << "Can't open dir: " << path << std::endl; return false;}

    std::string year = std::to_string(tm.tm_year + 1900);
    std::string month = std::to_string(tm.tm_mon + 1);
    std::string day = std::to_string(tm.tm_mday);
    std::string hour = std::to_string(tm.tm_hour);

    std::string mon_day_yaer_dir = path + "/" + month + "_" + day + "_" + year;
    if ( !CheckExitsAndMakeDir(mon_day_yaer_dir)) { std::cerr << "Can't open dir: " << mon_day_yaer_dir << std::endl; return false;}

    std::string hour_dir = mon_day_yaer_dir + "/" + hour;
    if ( !CheckExitsAndMakeDir(hour_dir)) { std::cerr << "Can't open dir: " << hour_dir << std::endl; return false;}

    event_dir = hour_dir + "/" + std::to_string(time_id);
    if ( !CheckExitsAndMakeDir(event_dir)) { std::cerr << "Can't open dir: " << event_dir << std::endl; return false;}

    return true;
}

//-------------------------------------------------------------------------------------
bool SaveText (const std::string & path, std::string & name, const std::string & jsonText, int64_t time_id)
{
    std::string event_dir;

    if (!MakeFolder (path, time_id, event_dir)) {
        return false;
    }

    //save json to text file
    std::string nameJson = event_dir + "/" + name + ".txt";
    std::ofstream out(nameJson);
    out << jsonText;
    out.close();
    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool SaveImage(const std::string & path, std::string & name, cv::Mat & img, int64_t time_id)
{
    std::string event_dir;

    if (!MakeFolder (path, time_id, event_dir)) {
        return false;
    }

    //save image
    std::string nameImg = event_dir + "/" + name + ".jpg";
    cv::imwrite(nameImg, img);

    return true;
}

//---------------------------------------------------------------------------------------------------------------
#include <sys/stat.h>
#include <iostream>

bool IsExists(const std::string & file)
{
    struct stat buf;
    return (stat(file.c_str(), &buf) == 0);
}

bool MakeDir(const std::string & dir)
{
    const int dir_err = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err) {
        std::cerr << "Error: Can't create dir: " << dir << std::endl;
        return false;
    }

    return true;
}

bool CheckExitsAndMakeDir(const std::string & dir)
{
    if ( !IsExists( dir )) {
        //        if(cfg.verbose_level >= 1) {
        //            std::cout << "Create dir: " << dir << std::endl;
        //        }
        if ( !MakeDir( dir )) {
            return false;
        }
    }
    return true;
}

