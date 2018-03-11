/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
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
