#include <stdio.h>
#include <string>

#include "libgconfig.h"

using namespace std;


Config::Config()
{

}

Config::~Config()
{

}

Config *Config::create(string path)
{
    string::size_type pos = path.rfind('.');
    if (pos == string::npos) {
        printf("%s is invalid.\n", path.c_str());
        return NULL;
    }
    string suffix = path.substr(pos+1);
    if (suffix == "lua") {
        return (Config *)LuaConfig::create(path.c_str());
    } else if (suffix == "json") {
        return (Config *)JsonConfig::create(path.c_str());
    } else {
        return NULL;
    }
}

