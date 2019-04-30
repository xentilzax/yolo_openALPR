
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "disk_adapter.hpp"

using namespace IZ;

//---------------------------------------------------------------------------------------------------------------
int GetDir (const std::string & dir, std::vector<std::string> & files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        std::cerr << "Error(" << errno << ") opening " << dir << std::endl;
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
    //    static std::set<char> delims{'\\','/','.','-'};
    //    std::vector<std::string> path = SplitPath(s, delims);

    //    if(path.size() >= 3) {
    return std::stoul(s);
    //    } else return 0;
}

//---------------------------------------------------------------------------------------------------------------
bool GetListEvents(const std::string & path, std::vector<IZ::DiskEventItem> & events)
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

                if( file_list.size() > 0 ) {
                    IZ::DiskEventItem item;
                    for(std::string & file_name : file_list ) {
                        file_name = path_event + "/" + file_name;
                    }
                    item.files = file_list;
                    item.timeLabel = GetTimeEvent(event);
                    events.push_back(item);
                }
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DeleteEmptyDir(const std::string & path)
{
    bool isChanged = true;

    while(isChanged) {
         std::vector<std::string> date_list;
         isChanged = false;

        if( GetDir(path, date_list) != 0 )
            return false;

        for(const std::string & date: date_list) {
            std::vector<std::string> hour_list;
            std:: string path_date = path + "/" + date;
            if( GetDir(path_date, hour_list) != 0 )
                return false;

            if( hour_list.empty()) {
                if( remove(path_date.c_str()) != 0 ) {
                    std::cerr << "Error deleting path " << path_date << std::endl;
                    return false;
                }
                isChanged = true;
            }

            for(const std::string & hour: hour_list) {
                std::vector<std::string> event_list;
                std::string path_hour = path_date + "/" + hour;
                if( GetDir(path_hour, event_list) != 0 )
                    return false;

                if( event_list.empty()) {
                    if( remove(path_hour.c_str()) != 0 ) {
                        std::cerr << "Error deleting path " << path_hour << std::endl;
                        return false;
                    }
                    isChanged = true;
                }

                for(const std::string & event: event_list) {
                    std:: string path_event = path_hour + "/" + event;
                    std::vector<std::string> file_list;
                    if( GetDir(path_event, file_list) != 0 )
                        return false;

                    if( file_list.empty()) {
                        if( remove(path_event.c_str()) != 0 ) {
                            std::cerr << "Error deleting path " << path_event << std::endl;
                            return false;
                        }
                        isChanged = true;
                    }
                }
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DeleteEvents(std::vector<IZ::DiskEventItem> & events, int count)
{
    if( events.size() == 0 ) {
        std::cerr << "Nothing to delete" << std::endl;
        return false;
    }

    for(size_t i = events.size() - 1; i >= events.size() - count; i--) {
        for(const std::string & file : events[i].files) {
            if( remove(file.c_str()) != 0 ) {
                std::cerr << "Error deleting file " << file << std::endl;
                return false;
            }
        }
    }

    events.resize(events.size() - count);

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DeleteOlderEvents(std::vector<IZ::DiskEventItem> & events, unsigned int count)
{
    //sort events by time mark
    std::sort(events.begin(), events.end(), [](const IZ::DiskEventItem & a, const IZ::DiskEventItem & b) { return a.timeLabel > b.timeLabel; });

    //delete older events
    return DeleteEvents(events, count);
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
    *freeSz /= 1024;

    return true;
}

//---------------------------------------------------------------------------------------------------------------
bool DiskCleaner::ControlDiskSpace()
{
    const std::string path = cfg.path;
    bool res = true;
    std::vector<IZ::DiskEventItem> events;
    unsigned long long freeSz;

    if( !GetListEvents(path, events) )
        return false;
    if( !GetSizeFreeDiskSpace(path, &freeSz) )
        return false;

    while( freeSz < cfg.min_size_free_space && events.size() > 0) {
        res &= DeleteOlderEvents(events, 1);
        res &= GetSizeFreeDiskSpace(path, &freeSz);
        if(!res)
            return false;
    }

    if( events.size() > cfg.max_event_number  && events.size() > 0) {
        res &= DeleteOlderEvents(events, events.size() - cfg.max_event_number);
        res &= GetSizeFreeDiskSpace(path, &freeSz);
        if(!res)
            return false;
    }

    res &= DeleteEmptyDir(path);

    return res;
}
