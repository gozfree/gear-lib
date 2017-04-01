#ifndef JSON_CONFIG_H
#define JSON_CONFIG_H

#include <jsoncpp/json/json.h>

class JsonConfig: public Json::Value
{
public:
    JsonConfig();
    ~JsonConfig();

public:
    static JsonConfig *create(const char *path);
    void destroy();

private:
    bool init(const char *path);
    JsonConfig& operator=(const Json::Value &conf);

};


#endif
