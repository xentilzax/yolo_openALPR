#include "socket_adapter.hpp"

using namespace IZ;

void SocketAdapter::ParseConfig(cJSON* json_sub, SocketAdapter_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "url");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.url = json_item->valuestring;
    }

}

//-------------------------------------------------------------------------------------
void SocketAdapter::SaveEvent(std::vector<EventMotionDetection> & event)
{

}

//-------------------------------------------------------------------------------------
void SocketAdapter::SaveEvent(std::vector<EventObjectDetection> & event)
{

}

//-------------------------------------------------------------------------------------
void SocketAdapter::SaveEvent(std::vector<EventObjectRecognize> & event)
{

}

