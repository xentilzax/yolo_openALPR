#include <algorithm>  //std::sort
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono> //sleep_for
#include <thread> //sleep_for

#include "config.hpp"

//---------------------------------------------------------------------------------------------------------------

IZ::Config cfg;

//---------------------------------------------------------------------------------------------------------------
bool ControlDiskSpace(const std::string & path);
void MillisecondsSleep(int sleepMs);

//---------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    std::string config_filename = "cfg/lprecognizer.cfg";
    std::ifstream fs(config_filename);
    if( !fs.good() ) {
        std::cout << "Error: Can't open config file: " << config_filename << std::endl;
        return 0x0A;
    }
    std::string str((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());

    std::cout << "JSON config: " << str << std::endl;
    std::cout << std::flush;
    ParseConfig(str, cfg);

    while(1) {
        if( !ControlDiskSpace(cfg.path) )
            return 0x0B;

        std::this_thread::sleep_for(std::chrono::milliseconds(cfg.removal_period_minutes*60*1000));
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

int GetDir (const std::string & dir, std::vector<std::string> & files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cout << "Error(" << errno << ") opening " << dir << std::endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string filename(dirp->d_name);
        if(filename == "." || filename == "..")
            continue;
        files.push_back(filename);
    }
    closedir(dp);
    return 0;
}

//---------------------------------------------------------------------------------------------------------------
std::vector<std::string> SplitPath(
        const std::string & str,
        const std::set<char> delimiters)
{
    std::vector<std::string> result;

    char const* pch = str.c_str();
    char const* start = pch;
    for(; *pch; ++pch)
    {
        if (delimiters.find(*pch) != delimiters.end())
        {
            if (start != pch)
            {
                std::string str(start, pch);
                result.push_back(str);
            }
            else
            {
                result.push_back("");
            }
            start = pch + 1;
        }
    }
    result.push_back(start);

    return result;
}

//---------------------------------------------------------------------------------------------------------------
// file name struct: <cam_id-event_id-type_image>.jpg

unsigned long long GetTimeEvent(const std::string & s)
{
    static std::set<char> delims{'\\','/','.','-'};
    std::vector<std::string> path = SplitPath(s, delims);

    if(path.size() >= 3) {
        return std::stoul(path[path.size()-3]);
    } else return 0;
}

//---------------------------------------------------------------------------------------------------------------
bool GetListEvents(const std::string & path, std::vector<IZ::Event> & events)
{
    static std::set<char> delims{'.'};
    std::vector<std::string> date_list;

    if( GetDir(path, date_list) != 0 )
        return false;

    for(const std::string & date: date_list) {
        std::vector<std::string> hour_list;
        std:: string path_date = path + "/" + date;
        if( GetDir(path_date, hour_list) != 0 )
            return false;

        for(const std::string & hour: hour_list) {
            std::vector<std::string> event_list;
            std:: string path_hour = path_date + "/" + hour;
            if( GetDir(path_hour, event_list) != 0 )
                return false;

            for(const std::string & event: event_list) {
                std:: string path_event = path_hour + "/" + event;
                std::vector<std::string> file_list;
                if( GetDir(path_event, file_list) != 0 )
                    return false;

                std::string file_name;
                if( file_list.size() == 2 ) {
                    std::vector<std::string> path_file = SplitPath(file_list[0], delims);
                    if(path_file.size() == 2) {
                        file_name = path_file[path_file.size() - 2];
                    } else {
                        return false;
                    }

                    IZ::Event evn;
                    evn.imagePath = path_event + "/" + file_name + ".jpg";
                    evn.textPath = path_event + "/" + file_name + ".txt";
                    evn.timeLabel = GetTimeEvent(evn.imagePath);
                    events.push_back(evn);

                }
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DeleteEmptyDir(const std::string & path)
{
    std::vector<std::string> date_list;

    if( GetDir(path, date_list) != 0 )
        return false;

    bool isChanged = true;

    while(isChanged) {
        isChanged = false;

        for(const std::string & date: date_list) {
            std::vector<std::string> hour_list;
            std:: string path_date = path + "/" + date;
            if( GetDir(path_date, hour_list) != 0 )
                return false;

            if( hour_list.empty()) {
                if( remove(path_date.c_str()) != 0 ) std::cerr << "Error deleting file " << path_date << std::endl;
                isChanged = true;
            }

            for(const std::string & hour: hour_list) {
                std::vector<std::string> event_list;
                std::string path_hour = path_date + "/" + hour;
                if( GetDir(path_hour, event_list) != 0 )
                    return false;

                if( event_list.empty()) {
                    if( remove(path_hour.c_str()) != 0 ) std::cerr << "Error deleting file " << path_hour << std::endl;
                    isChanged = true;
                }

                for(const std::string & event: event_list) {
                    std:: string path_event = path_hour + "/" + event;
                    std::vector<std::string> file_list;
                    if( GetDir(path_event, file_list) != 0 )
                        return false;

                    if( file_list.empty()) {
                        if( remove(path_event.c_str()) != 0 ) std::cerr << "Error deleting file " << path_event << std::endl;
                        isChanged = true;
                    }
                }
            }
        }
    }


    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DeleteOlderEvents(std::vector<IZ::Event> & events, int count)
{
    //sort events by time mark
    std::sort(events.begin(), events.end(), [](const IZ::Event & a, const IZ::Event & b) { return a.timeLabel > b.timeLabel; });

    //    std::cout << "--------------- Events -------------" << std::endl;
    //    for(const IZ::Event & event : events) {
    //        std::cout << event.imagePath << std::endl;
    //    }
    //    std::cout << "------------------------------------" << std::endl;


    //delete older events
    if( count > 0) {
        for(size_t i = events.size() - 1; i >= events.size() - count; i--) {
            if( remove(events[i].imagePath.c_str()) != 0 )
                std::cerr << "Error deleting file " << events[i].imagePath << std::endl;
            if( remove(events[i].textPath.c_str()) != 0 )
                std::cerr << "Error deleting file " << events[i].textPath << std::endl;
        }

        events.resize(events.size() - count);
    } else {
        events.resize(events.size() - 1);
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------
#include <sys/statvfs.h>

bool GetSizeFreeDiskSpace(const std::string & path, unsigned long long * freeSz)
{
    struct statvfs statFS;

    if( statvfs(path.c_str(), &statFS) != 0 ) {
        return false;
    }

    *freeSz = statFS.f_bsize * statFS.f_bavail;//f_bfree;
    *freeSz /= 1024 * 1024;

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool ControlDiskSpace(const std::string & path)
{
    bool res = true;
    std::vector<IZ::Event> events;
    unsigned long long freeSz;

    res &= GetListEvents(path, events);
    res &= GetSizeFreeDiskSpace(path, &freeSz);

    if(cfg.verbose_level > 0) {
        std::cout << "free space: " << freeSz << std::endl;
        std::cout << "number events: " << events.size() << std::endl;
    }

    while( freeSz < cfg.min_size_free_space || events.size() > cfg.max_event_number ) {

        DeleteOlderEvents(events, events.size() - cfg.max_event_number);

        res &= GetSizeFreeDiskSpace(path, &freeSz);

        if(cfg.verbose_level > 0) {
            std::cout << "free space: " << freeSz << std::endl;
            std::cout << "number events: " << events.size() << std::endl;
        }
    }

    res &= DeleteEmptyDir(path);

    return res;
}
