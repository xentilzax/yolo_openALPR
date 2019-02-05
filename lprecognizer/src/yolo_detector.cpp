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
void YOLO_Detector::Detection(const std::vector<IZ::EventMotionDetection> & events,
                              std::vector<IZ::EventObjectDetection> & results)
{
    for(size_t k = 0; k < events.size(); k++) {
        cv::Mat img = events[k].frame;
        std::vector<bbox_t> result_vec;
        result_vec = yolo->detect(img, cfg.yolo_thresh);

        EventObjectDetection eventObjectDetection(events[k]);

        for(size_t i = 0; i < result_vec.size(); i++) {
            bbox_t b = result_vec[i];
            ResultDetection rd;

            cv::Point point;
            point.x = b.x;
            point.y = b.y;
            rd.border.points.push_back(point);

            point.x = b.x + b.w;
            point.y = b.y;
            rd.border.points.push_back(point);

            point.x = b.x + b.w;
            point.y = b.y + b.h;
            rd.border.points.push_back(point);

            point.x = b.x;
            point.y = b.y + b.h;
            rd.border.points.push_back(point);

            rd.confdenceDetection = b.prob;

            cv::Rect roi(b.x, b.y, b.w, b.h);
            img(roi).copyTo(rd.croppedFrame);

            eventObjectDetection.detectedObjects.push_back(rd);
        }

        results.push_back(eventObjectDetection);
    }
}




