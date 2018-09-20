#include "config.hpp"

using namespace Inex;

//---------------------------------------------------------------------------------------------------------------
/*
{
  "yolo":
  {
      "neuralnet_config": "cfg/tiny_onebbox.cfg",
      "weights": "weights/tiny_onebbox_hik.weights",
      "threshold": 0.5
  },

  "open_alpr":
  {
      "config": "cfg/open_alpr.conf",
  },

  "http_server": "http://jsonplaceholder.typicode.com/posts",
  "camera": "rtsp://10.42.0.247",
  "gui": 0
}
*/

int ParseConfig(const std::string & str, Config & cfg)
{
    int status = 0;
    cJSON* json;
    cJSON* json_sub;
    cJSON* json_item;

    json = cJSON_Parse(str.c_str());
    if (json == NULL)
        goto end;

    //YOLO
    json_sub = cJSON_GetObjectItemCaseSensitive(json, "yolo");
    if (json_sub == NULL)
        goto end;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "neuralnet_config");
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

    //Open_ALPR
    json_sub = cJSON_GetObjectItemCaseSensitive(json, "open_alpr");
    if (json_sub == NULL)
        goto end;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config_skip_detection");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_skip_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "contry");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_contry = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "region");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_region = json_item->valuestring;
    }

    //LP Recognazer
    json_item = cJSON_GetObjectItemCaseSensitive(json, "use_yolo_detector");
    if (cJSON_IsNumber(json_item)) {
        cfg.use_yolo_detector = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "http_server");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.server = json_item->valuestring;
    }

    json_sub = cJSON_GetObjectItemCaseSensitive(json, "disk");
    if (json_sub == NULL)
        goto end;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "path_to_save");
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
