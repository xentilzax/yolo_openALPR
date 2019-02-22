#include "yolo_detector.hpp"

using namespace IZ;

//-----------------------------------------------------------------------------------------------------
YOLO_Detector::YOLO_Detector(cJSON* json)
{
    YOLO_Detector::ParseConfig(json, cfg);
    Init();
}
//-----------------------------------------------------------------------------------------------------
YOLO_Detector::YOLO_Detector(const YOLO_Config & conf)
{
    cfg = conf;
    Init();
}

//-----------------------------------------------------------------------------------------------------
void YOLO_Detector::ParseConfig(cJSON* json_sub, YOLO_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.yolo_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "weights");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.yolo_weights = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "threshold");
    if (cJSON_IsNumber(json_item)) {
        cfg.yolo_thresh = json_item->valuedouble;
    }
}

//-----------------------------------------------------------------------------------------------------
void YOLO_Detector::Init()
{
    yolo = std::shared_ptr<Detector>(new Detector(cfg.yolo_cfg, cfg.yolo_weights));
}


//-----------------------------------------------------------------------------------------------------
void YOLO_Detector::Detection(std::shared_ptr<IZ::Event> & event)
{
    std::shared_ptr<EventObjectDetection> eventObjectDetection = std::make_shared<EventObjectDetection>(event.get());

    for(size_t k = 0; k < eventObjectDetection->arrayResults.size(); k++) {

        cv::Mat img = std::dynamic_pointer_cast<ResultMotion>(eventObjectDetection->arrayResults[k])->frame;
        std::vector<bbox_t> result_vec;

        result_vec = yolo->detect(img, cfg.yolo_thresh);

        std::shared_ptr<ResultDetection> rd =
                std::make_shared<ResultDetection>(
                    new ResultDetection(
                        std::dynamic_pointer_cast<ResultMotion>(
                            eventObjectDetection->arrayResults[k]).get()));

        for(size_t i = 0; i < result_vec.size(); i++) {
            bbox_t b = result_vec[i];
            std::shared_ptr<DataDetection> data = std::make_shared<DataDetection>(rd.get(), i);

            cv::Point point;
            point.x = b.x;
            point.y = b.y;
            data->border.points.push_back(point);

            point.x = b.x + b.w;
            point.y = b.y;
            data->border.points.push_back(point);

            point.x = b.x + b.w;
            point.y = b.y + b.h;
            data->border.points.push_back(point);

            point.x = b.x;
            point.y = b.y + b.h;
            data->border.points.push_back(point);

            data->confdenceDetection = b.prob;

            cv::Rect roi(b.x, b.y, b.w, b.h);
            img(roi).copyTo(data->croppedFrame);

            rd->objectData.push_back(data);
        }

        eventObjectDetection->arrayResults[k] = std::static_pointer_cast<Result>(rd);
    }

    event = std::static_pointer_cast<Event>(eventObjectDetection);
}




