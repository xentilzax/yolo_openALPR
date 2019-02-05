#include "config.hpp"

using namespace IZ;

//---------------------------------------------------------------------------------------------------------------
int ParseConfig(const std::string & str, Config & cfg)
{
    int status = 0;
    cJSON* json;
    cJSON* json_sub;
    cJSON* json_item;
    cJSON* json_arr;
    cJSON* json_recognizers;
    cJSON* json_detectors;
    cJSON* json_motion;
    cJSON* json_savers;
    cJSON* json_conveer;

    json = cJSON_Parse(str.c_str());
    if (json == NULL)
        goto end;

    //order load config sections impotant for Open_ALPR module!!!!
    json_recognizers = cJSON_GetObjectItemCaseSensitive(json, "recognizers");
    if (json_recognizers == NULL)
        goto end;
    {
        //Open_ALPR
        json_sub = cJSON_GetObjectItemCaseSensitive(json_recognizers, "open_alpr");
        if (json_sub == NULL)
            goto end;

        ALPR_Module::ParseConfig(json_sub, cfg);
    }

    json_detectors = cJSON_GetObjectItemCaseSensitive(json, "detectors");
    if (json_detectors == NULL)
        goto end;
    {
        //YOLO
        json_sub = cJSON_GetObjectItemCaseSensitive(json_detectors, "yolo");
        if (json_sub == NULL)
            goto end;

        YOLO_Detector::ParseConfig(json_sub, cfg);

        //Open_ALPR
        json_sub = cJSON_GetObjectItemCaseSensitive(json_detectors, "open_alpr");
        if (json_sub == NULL)
            goto end;

        ALPR_Module::ParseConfig(json_sub, cfg);

    }

    json_motion = cJSON_GetObjectItemCaseSensitive(json, "motion_detectors");
    if (json_motion == NULL)
        goto end;
    {
        //SIMPLE
        json_sub = cJSON_GetObjectItemCaseSensitive(json_motion, "simple");
        if (json_sub == NULL)
            goto end;
        {

        }
        SimpleMotion::ParseConfig(json_sub, cfg);

        //MoveDetector
        json_sub = cJSON_GetObjectItemCaseSensitive(json_motion, "background_substruction");
        if (json_sub == NULL)
            goto end;

        MoveDetector::ParseConfig(json_sub, cfg);
    }

    json_savers = cJSON_GetObjectItemCaseSensitive(json, "save_data_adapters");
    if (json_savers == NULL)
        goto end;
    {
        json_sub = cJSON_GetObjectItemCaseSensitive(json_savers, "disk_adapter");
        if (json_sub == NULL)
            goto end;

        DiskAdapter::ParseConfig(json_sub, cfg);

        json_sub = cJSON_GetObjectItemCaseSensitive(json_savers, "socket_adapter");
        if (json_sub == NULL)
            goto end;

        SocketAdapter::ParseConfig(json_sub, cfg);
    }

    json_conveer = cJSON_GetObjectItemCaseSensitive(json, "conveer");
    if (json_motion == NULL)
        goto end;
    {
        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "motion_detector_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.motionDetectorName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "motion_detector_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.motionDetectorAdapters.push_back(json_sub->valuestring);
                }
            }
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "detector_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.detectorName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "detector_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.detectorAdapters.push_back(json_sub->valuestring);
                }
            }
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "recognizer_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.recognizerName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "recognizer_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.recognizerAdapters.push_back(json_sub->valuestring);
                }
            }
        }
    }
    //LP Recognazer

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_url");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_url = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_id");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_id = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_type");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_type = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "gui");
    if (cJSON_IsNumber(json_item)) {
        cfg.gui_enable = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "verbose_level");
    if (cJSON_IsNumber(json_item)) {
        cfg.verbose_level = json_item->valueint;
    }

    cJSON_Delete(json);
    return 1;

end:
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
        fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    status = 0;

    cJSON_Delete(json);
    return status;
}
