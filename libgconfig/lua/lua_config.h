#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include "luatables.h"

using namespace std;

class LuaConfig: public LuaTable
{
public:
    LuaConfig();
    ~LuaConfig();

public:
    static LuaConfig *create(const char *path);
    void destroy();
    bool save();

private:
    bool init(const char *config);
    LuaConfig& operator=(const LuaTable &table);
};


#endif
