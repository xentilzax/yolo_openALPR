#include "socket_adapter.hpp"


using namespace IZ;

//-------------------------------------------------------------------------------------
SocketThread* SocketThread::ptrThis = nullptr;

std::mutex SocketThread::mutexSave;
std::queue <std::shared_ptr<Event> > SocketThread::queueEvents;

//-------------------------------------------------------------------------------------
void SocketAdapter::ParseConfig(cJSON* json_sub, SocketAdapter_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "url");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.url = json_item->valuestring;
    }

}

//-------------------------------------------------------------------------------------
void SocketAdapter::SaveEvent(std::shared_ptr<Event> event)
{
    if(event->arrayResults.empty())
        return;

    std::lock_guard<std::mutex> guard(localPtr->mutexSave);
    localPtr->queueEvents.push(event);
}


//-------------------------------------------------------------------------------------
bool SocketThread::QueueIsEmpty(std::shared_ptr<Event> & event)
{
    std::lock_guard<std::mutex> guard(mutexSave);
    if( queueEvents.empty() ) {
        return true;
    }

    event = queueEvents.front();
    queueEvents.pop();
    return false;
}

//-------------------------------------------------------------------------------------
void SocketThread::Sending(SocketThread* ptr)
{
    while(ptr->enableWorking) {
        std::shared_ptr<Event> event;

        if(QueueIsEmpty(event)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        cJSON * json = cJSON_CreateObject();
        cJSON * jsonItem = event->GenerateJson();
        cJSON_AddItemToObject(json, "event", jsonItem);

        std::unique_ptr<char, std::function<void(char*)>> textJson( cJSON_Print(json), [](char* ptr){cJSON_free(ptr);} );

        cJSON_Delete(json);

        if (textJson.get() == NULL) {
            std::cerr << "Failed to print cJSON\n";
            return;
        }

        if(PostHTTP(ptr->cfg.url, textJson.get(), 3) != 1)
            throw std::runtime_error(("Can't send text to url: " + ptr->cfg.url).c_str());
    }
}
