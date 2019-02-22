#pragma once

#include <string>
#include <vector>

#include "disk_adapter.hpp"

namespace IZ {
class DiskAdapter_Config;

//-------------------------------------------------------------------------------------
class DiskEventItem
{
public:
    std::vector<std::string> files;
    unsigned long long timeLabel;
};

//-------------------------------------------------------------------------------------
class DiskCleaner
{
public:
    DiskCleaner(const DiskAdapter_Config & conf)
        : cfg(conf)
    {}
    bool ControlDiskSpace();

    DiskAdapter_Config cfg;
};

}
