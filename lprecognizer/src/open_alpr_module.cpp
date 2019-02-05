#include "cJSON.h"
#include "open_alpr_module.hpp"

using namespace IZ;


//template<typename T, typename... Args>
//std::unique_ptr<T> make_unique(Args&&... args) {
//    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
//}

std::shared_ptr<alpr::Alpr> ALPR_Module::openalpr;

//-----------------------------------------------------------------------------------------------------
ALPR_Module::ALPR_Module(cJSON* json)
{
    ParseConfig(json, cfg);
    Init();
}
//-----------------------------------------------------------------------------------------------------
ALPR_Module::ALPR_Module(const ALPR_Config & conf)
{
    cfg = conf;
    Init();
}

//-----------------------------------------------------------------------------------------------------
void ALPR_Module::ParseConfig(cJSON* json_sub, ALPR_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "contry");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_contry = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "region");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_region = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "number_variations");
    if (cJSON_IsNumber(json_item)) {
        cfg.open_alpr_num_variation = json_item->valueint;
    }
}

//-----------------------------------------------------------------------------------------------------
void ALPR_Module::Init()
{
    if(!openalpr) {
        openalpr = std::shared_ptr<alpr::Alpr>(new alpr::Alpr(cfg.open_alpr_contry, cfg.open_alpr_cfg));
        // Optionally, you can specify the top N possible plates to return (with confidences). The default is ten.
        openalpr->setTopN(cfg.open_alpr_num_variation);
        // Optionally, you can provide the library with a region for pattern matching. This improves accuracy by
        // comparing the plate text with the regional pattern.
        openalpr->setDefaultRegion(cfg.open_alpr_region);

        // Make sure the library loads before continuing.
        // For example, it could fail if the config/runtime_data is not found.
        if (openalpr->isLoaded() == false) {
            std::cerr << "Error loading OpenALPR" << std::endl;
            throw std::string();
        }
    }
}

//-----------------------------------------------------------------------------------------------------
void ALPR_Module::Detection(const std::vector<EventMotionDetection> & events,
                            std::vector<EventObjectDetection> & results)
{
    open_alpr_use_detector = true;
    alprResults.clear();

    for(size_t k = 0; k < events.size(); k++) {
        std::vector<alpr::AlprRegionOfInterest> roi_list;
        cv::Mat img = events[k].frame;
        alpr::AlprResults alprResult;

        alprResult = openalpr->recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);

        EventObjectDetection eventObjectDetection(events[k]);

        for(size_t i = 0; i < alprResult.plates.size(); i++) {
            ResultDetection rd;

            for(size_t j = 0; j < 4; j++) {
                cv::Point point;
                point.x = alprResult.plates[i].plate_points[j].x;
                point.y = alprResult.plates[i].plate_points[j].y;
                rd.border.points.push_back(point);
            }
            rd.confdenceDetection = alprResult.plates[i].regionConfidence / 100.f;
            rd.croppedFrame = cv::imdecode(cv::Mat(alprResult.plates[i].plate_crop_jpeg), cv::IMREAD_COLOR);
            eventObjectDetection.detectedObjects.push_back(rd);
        }
        alprResults.push_back(alprResult);
        results.push_back(eventObjectDetection);
    }
}

//-----------------------------------------------------------------------------------------------------
void ALPR_Module::Recognize(std::vector<IZ::EventObjectDetection> & events,
                            std::vector<IZ::EventObjectRecognize> & result)
{
    if(open_alpr_use_detector) {
        for(size_t k = 0; k < events.size(); k++) {
            EventObjectRecognize eventObjectRecognize(events[k]);

            for(size_t i = 0; i < events[k].detectedObjects.size(); i++) {
                ResultRecognition rr(events[k].detectedObjects[i]);

                rr.bestPlate.characters = alprResults[k].plates[i].bestPlate.characters;
                rr.bestPlate.confidence = alprResults[k].plates[i].bestPlate.overall_confidence / 100.f;
                rr.bestPlate.matchesTemplate = alprResults[k].plates[i].bestPlate.matches_template;

                for(const auto & pl : alprResults[k].plates[i].topNPlates) {
                    Plate plate;
                    plate.characters = pl.characters;
                    plate.confidence = pl.overall_confidence / 100.f;
                    plate.matchesTemplate = pl.matches_template;
                    rr.topNPlates.push_back(plate);
                }

                eventObjectRecognize.recognizedObjects.push_back(rr);
            }

            result.push_back(eventObjectRecognize);
        }

    } else {
        alprResults.clear();
        std::vector<alpr::AlprRegionOfInterest> roi_list;

        for(size_t k = 0; k < events.size(); k++) {
            EventObjectRecognize eventObjectRecognize(events[k]);

            for(size_t i = 0; i < events[k].detectedObjects.size(); i++) {
                cv::Mat img = events[k].detectedObjects[i].croppedFrame;

                alpr::AlprResults alprResult;
                alprResult = openalpr->recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);

                ResultRecognition rr(events[k].detectedObjects[i]);
                if(alprResult.plates.size() > 0) {

                    rr.bestPlate.characters = alprResult.plates[0].bestPlate.characters;
                    rr.bestPlate.confidence = alprResult.plates[0].bestPlate.overall_confidence / 100.f;
                    rr.bestPlate.matchesTemplate = alprResult.plates[0].bestPlate.matches_template;
                    rr.recognized = true;

                    for(const auto & pl : alprResult.plates[0].topNPlates) {
                        Plate plate;
                        plate.characters = pl.characters;
                        plate.confidence = pl.overall_confidence / 100.f;
                        plate.matchesTemplate = pl.matches_template;
                        rr.topNPlates.push_back(plate);
                    }
                } else {
                    rr.recognized = false;
                }
                eventObjectRecognize.recognizedObjects.push_back(rr);
            }

            result.push_back(eventObjectRecognize);
        }
    }
}
