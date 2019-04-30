#pragma once

/* http://curl.haxx.se/download.html */
#include <curl/curl.h>
#include <string>


int PostHTTP(const std::string & url, const char *json, int verbose_level);
