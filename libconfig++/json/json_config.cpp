#include <fstream>
#include "json_config.h"
using namespace std;


JsonConfig::JsonConfig()
{
}

JsonConfig::~JsonConfig()
{
}

JsonConfig *JsonConfig::create(const char *path)
{
    JsonConfig *result = new JsonConfig();
    if (result && !result->init(path)) {
        delete result;
        result = NULL;
    }
    return result;
}

bool JsonConfig::init(const char *path)
{
    ifstream ifs;
    ifs.open(path);
    if (!ifs.is_open()) {
        return false;
    }
    Json::Reader m_reader;
    if (!m_reader.parse(ifs, *this, false)) {
        return false;
    }
    return true;
}

void JsonConfig::destroy()
{
    delete this;
}

JsonConfig& JsonConfig::operator=(const Json::Value &conf)
{
    *((Json::Value*)this) = conf;
    return *this;
}
