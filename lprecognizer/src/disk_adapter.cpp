#include "disk_adapter.hpp"

#include <iostream>
#include <fstream>
#include <thread>         // std::thread

using namespace IZ;

//-------------------------------------------------------------------------------------
SavingThread* SavingThread::ptrThis = nullptr;

std::mutex SavingThread::mutexSave;
std::queue <std::shared_ptr<Event> > SavingThread::queueEvents;
//static std::atomic<bool> enableWorking = true;

//-------------------------------------------------------------------------------------
bool MakeFolder (const std::string & path, int64_t time_id, std::string & event_dir);
bool CheckExitsAndMakeDir(const std::string & dir);


//-------------------------------------------------------------------------------------
void DiskAdapter::ParseConfig(cJSON* json_sub, DiskAdapter_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "path");
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
}

//-------------------------------------------------------------------------------------
bool SavingThread::QueueIsEmpty(std::shared_ptr<Event> & event)
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
void SavingThread::Saving(SavingThread* ptr)
{
    milliseconds startTime = duration_cast<milliseconds>( system_clock::now().time_since_epoch() );
    milliseconds currTime;

    while(ptr->enableWorking) {
        currTime = duration_cast<milliseconds>( system_clock::now().time_since_epoch() );

        if(duration_cast<milliseconds>(currTime - startTime).count() > cfg.removal_period_minutes*60*1000) {
            if(ptr->diskCleaner.ControlDiskSpace())
                throw std::runtime_error("Error cleaning disk");
        }

        std::shared_ptr<Event> event;

        if(QueueIsEmpty(event)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        std::string eventDir;
        ptr->MakeEventFolder(event, eventDir);

        cJSON * json = cJSON_CreateObject();
        cJSON * jsonItem = event->GenerateJson();
        cJSON_AddItemToObject(json, "event", jsonItem);

        std::unique_ptr<char[]> textJson( cJSON_Print(json) );

        cJSON_Delete(json);

        if (textJson.get() == NULL) {
            std::cerr << "Failed to print cJSON\n";
            return;
        }

        std::string textFilename = eventDir + "/data.txt";
        std::ofstream textFile;
        textFile.open(textFilename);
        textFile << std::string(textJson.get());
        textFile.close();

        event->SaveImages(eventDir);
    }
}

//-------------------------------------------------------------------------------------
void DiskAdapter::SaveEvent(std::shared_ptr<Event> event)
{
    if(event->arrayResults.empty())
        return;

    std::lock_guard<std::mutex> guard(localPtr->mutexSave);
    localPtr->queueEvents.push(event);
}


//-------------------------------------------------------------------------------------
void SavingThread::MakeEventFolder(const std::shared_ptr<Event> event, std::string & eventDir)
{
    //find minimal timestamp from events
    int64_t minTimestamp = event->GetTimestamp();

    if (!MakeFolder (cfg.path, minTimestamp, eventDir)) {
        throw std::runtime_error("Can't make folder for timestamp: " + std::to_string(minTimestamp) + " in default path: " + cfg.path);
    }
}

//-------------------------------------------------------------------------------------
bool MakeFolder (const std::string & path, int64_t time_id, std::string & event_dir)
{
    time_t t = static_cast<time_t>(time_id/1000);

    std::tm tm = *std::localtime(&t);

    if ( !CheckExitsAndMakeDir(path)) { std::cerr << "Can't open dir: " << path << std::endl; return false;}

    std::string year = std::to_string(tm.tm_year + 1900);
    std::string month = std::to_string(tm.tm_mon + 1);
    std::string day = std::to_string(tm.tm_mday);
    std::string hour = std::to_string(tm.tm_hour);

    std::string mon_day_yaer_dir = path + "/" + month + "_" + day + "_" + year;
    if ( !CheckExitsAndMakeDir(mon_day_yaer_dir)) { std::cerr << "Can't open dir: " << mon_day_yaer_dir << std::endl; return false;}

    std::string hour_dir = mon_day_yaer_dir + "/" + hour;
    if ( !CheckExitsAndMakeDir(hour_dir)) { std::cerr << "Can't open dir: " << hour_dir << std::endl; return false;}

    event_dir = hour_dir + "/" + std::to_string(time_id);
    if ( !CheckExitsAndMakeDir(event_dir)) { std::cerr << "Can't open dir: " << event_dir << std::endl; return false;}

    return true;
}

//---------------------------------------------------------------------------------------------------------------
#include <sys/stat.h>
#include <iostream>

bool IsExists(const std::string & file)
{
    struct stat buf;
    return (stat(file.c_str(), &buf) == 0);
}

bool MakeDir(const std::string & dir)
{
    const int dir_err = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err) {
        std::cerr << "Error: Can't create dir: " << dir << std::endl;
        return false;
    }

    return true;
}

bool CheckExitsAndMakeDir(const std::string & dir)
{
    if ( !IsExists( dir )) {
        //        if(cfg.verbose_level >= 1) {
        //            std::cout << "Create dir: " << dir << std::endl;
        //        }
        if ( !MakeDir( dir )) {
            return false;
        }
    }
    return true;
}

