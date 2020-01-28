# Gear-Lib

[English](README.md) | 简体中文

[![Build](https://travis-ci.org/gozfree/gear-lib.svg?branch=master)](https://travis-ci.org/gozfree/gear-lib)
[![Release](https://img.shields.io/github/release/gozfree/gear-lib.svg)](https://github.com/gozfree/gear-lib/releases)
[![License](https://img.shields.io/github/license/gozfree/gear-lib.svg)](https://github.com/gozfree/gear-lib/blob/master/LICENSE.MIT)

这是一组通用的Ｃ基础库
* 全部用POSIX C实现，目标是为了跨平台兼容linux, windows, android, ios.
* 适用于物联网，嵌入式，以及网络服务开发等场景

## 数据结构
* libdict: 哈希字典
* libhash: linux内核原生哈希库
* libringbuffer: 循环缓冲
* libqueue: 数据队列
* librbtree: 内核rbtree
* libsort:
* libvector: 容器库
* libmacro: 通用宏定义
* libdarray: 动态数组

## 网络库
* librtsp: RTSP协议，适合IPCamera和NVR开发
* librtmpc: RTMP协议，适合推流直播
* libskt: Socket封装
* librpc: 远程过程调用库
* libipc: 进程间通信
* libp2p: p2p穿透传输
* libhomekit: Apple homekit协议库

## 异步
* libgevent: 事件驱动
* libthread: 线程
* libworkq: 工作队列

## I/O
* libbase64: Base64/32 编解码
* libconfig: 配置文件库
* liblog: 日志库
* libfile: 文件操作库
* libstrex:
* libsubmask: 网络地址翻译

## 多媒体
* libuvc: USB摄像头库
* libmp4parser: MP4解析库
* libjpeg-ex:
* libmedia-io: 音频视频格式定义

## 系统抽象层
* libposix4win: windows平台poxix适配库
* libposix4rtos: FreeRTOS平台poxix适配库

## 其他
* libdebug: 调试辅助库
* libhal: 硬件抽象层
* libplugin: 动态加载库
* libtime: 时间库
* libfsm: 有限状态机

## 编译方法
详细请参考[INSTALL.md](https://github.com/gozfree/gear-lib/blob/master/INSTALL.md)

## License
详细请参考[LICENSE](https://github.com/gozfree/gear-lib/blob/master/LICENSE.MIT)

## 联系交流
* 邮箱: gozfree@163.com
* QQ 群: 695515645
* Github: [gear-lib](https://github.com/gozfree/gear-lib)
* 码云主页: [gear-lib](https://gitee.com/gozfreee/gear-lib)
