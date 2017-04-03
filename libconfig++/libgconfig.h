/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    libgconfig.h
 * author:  gozfree <gozfree@163.com>
 * created: 2017-03-30 15:00:24
 * updated: 2017-03-30 15:00:24
 *****************************************************************************/
#ifndef LIBGCONFIG_H
#define LIBGCONFIG_H

#include "lua/lua_config.h"
#include "json/json_config.h"

class Config : public LuaConfig, public JsonConfig
{
public:
    Config();
    ~Config();

public:
    static Config *create(string path);

};

#endif
