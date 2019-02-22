#pragma once

#include <string>
#include <atomic>         // std::atomic
#include <queue>          // std::queue
#include <memory>
#include <thread>
#include <mutex>

#include "cJSON.h"
#include "save_adapter.hpp"
#include "disk_cleaner.hpp"

namespace IZ {

//-------------------------------------------------------------------------------------
class DiskAdapter_Config
{
public:
    std::string path = "";
    unsigned int min_size_free_space = 100; //minimal free space on disk (Mb)
    unsigned int max_event_number = 1000;
    unsigned int removal_period_minutes = 10;
};


//-------------------------------------------------------------------------------------
//Singlton
class SavingThread
{
    DiskAdapter_Config cfg;
    std::atomic<bool> enableWorking;
    std::thread savingThread;
    DiskCleaner diskCleaner;


    static SavingThread* ptrThis;


    SavingThread(const DiskAdapter_Config & conf)
        : cfg(conf)
        , enableWorking(true)
        , savingThread(Saving, this)
        , diskCleaner(conf)
    {}

    void MakeEventFolder(const std::shared_ptr<Event> event, std::string & eventDir);

public:
    static SavingThread* Create(const DiskAdapter_Config & conf)
    {
        if(ptrThis == nullptr ) {
            ptrThis = new SavingThread(conf);
        }
        return ptrThis;
    }


    virtual ~SavingThread()
    {
        enableWorking = false;
        savingThread.join();
        ptrThis = nullptr;
    }

    static std::mutex mutexSave;
    static std::queue <std::shared_ptr<Event> > queueEvents;
    static void Saving(SavingThread* ptr);
    static bool QueueIsEmpty(std::shared_ptr<Event> & event);
};

//-------------------------------------------------------------------------------------
class DiskAdapter :public SaveAdapter
{

public:
    DiskAdapter(const DiskAdapter_Config & conf)
    {
        localPtr = std::shared_ptr<SavingThread>(SavingThread::Create(conf));
    }

    ~DiskAdapter() {}

    void SaveEvent(std::shared_ptr<Event> event);
    static void ParseConfig(cJSON* json_sub, DiskAdapter_Config & cfg);

private:
    std::shared_ptr<SavingThread> localPtr;
};

}
