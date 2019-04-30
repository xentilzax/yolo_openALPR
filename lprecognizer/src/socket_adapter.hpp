#pragma once

#include <string>
#include <atomic>         // std::atomic
#include <queue>          // std::queue
#include <memory>
#include <thread>
#include <mutex>

#include "cJSON.h"
#include "save_adapter.hpp"
#include "post.hpp"

namespace IZ {

class SocketAdapter_Config
{
public:
    std::string url = "";
};

//Singlton
class SocketThread
{
    SocketAdapter_Config cfg;
    std::atomic<bool> enableWorking;
    std::thread savingThread;

    static SocketThread* ptrThis;


    SocketThread(const SocketAdapter_Config & conf)
        : cfg(conf)
        , enableWorking(true)
        , savingThread(Sending, this)
    {
        if(curl_global_init(CURL_GLOBAL_ALL)) {
            throw std::runtime_error("Fatal: The initialization of libcurl has failed.\n");
        }

        if(atexit(curl_global_cleanup)) {
            curl_global_cleanup();
            throw std::runtime_error("Fatal: atexit failed to register curl_global_cleanup.\n");
        }
    }

public:
    static SocketThread* Create(const SocketAdapter_Config & conf)
    {
        if(ptrThis == nullptr ) {
            ptrThis = new SocketThread(conf);
        }
        return ptrThis;
    }


    virtual ~SocketThread()
    {
        enableWorking = false;
        savingThread.join();
        ptrThis = nullptr;
    }

    static std::mutex mutexSave;
    static std::queue <std::shared_ptr<Event> > queueEvents;
    static void Sending(SocketThread* ptr);
    static bool QueueIsEmpty(std::shared_ptr<Event> & event);
};

class SocketAdapter :public SaveAdapter
{
public:
    SocketAdapter(const SocketAdapter_Config & conf)
    {
        localPtr = std::shared_ptr<SocketThread>(SocketThread::Create(conf));
    }
    ~SocketAdapter() {}
    void SaveEvent(std::shared_ptr<Event> event);

    static void ParseConfig(cJSON* json_sub, SocketAdapter_Config & cfg);



private:
    std::shared_ptr<SocketThread> localPtr;
};

}
