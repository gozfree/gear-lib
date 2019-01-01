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
