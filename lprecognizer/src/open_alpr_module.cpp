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
void ALPR_Module::Detection(std::shared_ptr<Event> & event)
{
    open_alpr_use_detector = true;
    alprResults.clear();
    std::shared_ptr<EventObjectDetection> eventObjectDetection = std::make_shared<EventObjectDetection>(event.get());

    for(size_t k = 0; k < eventObjectDetection->arrayResults.size(); k++) {
        std::vector<alpr::AlprRegionOfInterest> roi_list;
        alpr::AlprResults alprResult;
        cv::Mat img = std::dynamic_pointer_cast<ResultMotion>(eventObjectDetection->arrayResults[k])->frame;

        alprResult = openalpr->recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);

        std::shared_ptr<ResultDetection> rd(
                    new ResultDetection(
                        reinterpret_cast<ResultMotion*>(
                            eventObjectDetection->arrayResults[k].get())));

        for(size_t i = 0; i < alprResult.plates.size(); i++) {
            std::shared_ptr<DataDetection> data =std::make_shared<DataDetection>(rd.get(), i);

            for(size_t j = 0; j < 4; j++) {
                cv::Point point;
                point.x = alprResult.plates[i].plate_points[j].x;
                point.y = alprResult.plates[i].plate_points[j].y;
                data->border.points.push_back(point);
            }
            data->confdenceDetection = alprResult.plates[i].regionConfidence / 100.f;
            data->croppedFrame = cv::imdecode(cv::Mat(alprResult.plates[i].plate_crop_jpeg), cv::IMREAD_COLOR);
            rd->objectData.push_back(data);
        }
        alprResults.push_back(alprResult);
        eventObjectDetection->arrayResults[k] = rd;
    }

    event = eventObjectDetection;
}

//-----------------------------------------------------------------------------------------------------
void ALPR_Module::Recognize(std::shared_ptr<Event> & event)
{
    std::shared_ptr<EventObjectRecognize> eventObjectRecognize = std::make_shared<EventObjectRecognize>(event.get());

    if(open_alpr_use_detector) {
        for(size_t k = 0; k < eventObjectRecognize->arrayResults.size(); k++) {
            std::shared_ptr<ResultDetection> rd = std::dynamic_pointer_cast<ResultDetection>(eventObjectRecognize->arrayResults[k]);

            for(size_t i = 0; i < rd->objectData.size(); i++) {
                const DataDetection* dd = reinterpret_cast<DataDetection*>(rd->objectData[i].get());
                std::shared_ptr<DataRecognition> data(new DataRecognition(dd));

                data->bestPlate.characters = alprResults[k].plates[i].bestPlate.characters;
                data->bestPlate.confidence = alprResults[k].plates[i].bestPlate.overall_confidence / 100.f;
                data->bestPlate.matchesTemplate = alprResults[k].plates[i].bestPlate.matches_template;

                for(const auto & pl : alprResults[k].plates[i].topNPlates) {
                    Plate plate;
                    plate.characters = pl.characters;
                    plate.confidence = pl.overall_confidence / 100.f;
                    plate.matchesTemplate = pl.matches_template;
                    data->topNPlates.push_back(plate);
                }

                rd->objectData[i] = std::static_pointer_cast<DataObject>(data);
            }
        }
    } else {
        alprResults.clear();
        std::vector<alpr::AlprRegionOfInterest> roi_list;

        for(size_t k = 0; k < eventObjectRecognize->arrayResults.size(); k++) {
            std::shared_ptr<ResultDetection> rd = std::dynamic_pointer_cast<ResultDetection>(eventObjectRecognize->arrayResults[k]);

            for(size_t i = 0; i < rd->objectData.size(); i++) {
                DataDetection* dd = reinterpret_cast<DataDetection*>(rd->objectData[i].get());
                std::shared_ptr<DataRecognition> data(new DataRecognition(dd));

                cv::Mat img = dd->croppedFrame;

                alpr::AlprResults alprResult;
                alprResult = openalpr->recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);

                if(alprResult.plates.size() > 0) {
                    data->bestPlate.characters = alprResult.plates[0].bestPlate.characters;
                    data->bestPlate.confidence = alprResult.plates[0].bestPlate.overall_confidence / 100.f;
                    data->bestPlate.matchesTemplate = alprResult.plates[0].bestPlate.matches_template;
                    data->recognized = true;

                    for(const auto & pl : alprResult.plates[0].topNPlates) {
                        Plate plate;
                        plate.characters = pl.characters;
                        plate.confidence = pl.overall_confidence / 100.f;
                        plate.matchesTemplate = pl.matches_template;
                        data->topNPlates.push_back(plate);
                    }
                } else {
                    data->recognized = false;
                }
                rd->objectData[i] = std::static_pointer_cast<DataObject>(data);
            }
        }
    }

    event = eventObjectRecognize;
}
