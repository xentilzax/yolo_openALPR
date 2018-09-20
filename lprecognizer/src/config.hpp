#pragma once

#include "common.hpp"
#include "cJSON.h"

int ParseConfig(const std::string & str, Inex::Config & cfg);
