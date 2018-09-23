libraries [English](README.md) [![Build Status](https://travis-ci.org/gozfree/libraries.svg?branch=master)](https://travis-ci.org/gozfree/libraries) [![Release](https://img.shields.io/github/release/gozfree/libraries.svg)](https://github.com/gozfree/libraries/releases) [![License](https://img.shields.io/github/license/gozfree/libraries.svg)](https://github.com/gozfree/libraries/blob/master/LICENSE.LGPL)
=========

libraries是一组通用的Ｃ基础库
* 全部用POSIX C实现，目标是为了跨平台兼容x86, arm, android, ios.
* 每个库尽可能各自独立，而且API容易使用
* 尽量不依赖任何第三方库
* 目标是为减少重复造轮子

|name|descript|name|descript|name|descript|
|----|--------|----|--------|----|--------|
|[libatomic](libatomic)|原子操作库|[libbase64](libbase64)|Base64/32 编解码|[libcmd](libcmd)|命令行库
|[libconfig](libconfig)|配置文件库|[libdebug](libdebug)|调试辅助库|[libdict](libdict)|哈希字典
|[libfilewatcher](libfilewatcher)|文件监控|[libfile](libfile)|文件操作库|[libgevent](libgevent)|事件驱动
|[libhal](libhal)|硬件抽象层|[libhash](libhash)|linux内核原生哈希库|[libipc](libipc)|进程间通信
|[liblog](liblog)|日志库|[libmacro](libmacro)|通用宏定义|[libmp4parser](libmp4parser)|MP4解析库
|[liblock](liblock)|锁操作|[libthread](libthread)|线程|[libp2p](libp2p)|p2p穿透传输
|[libplugin](libplugin)|动态加载库|[libptcp](libptcp)|TCP协议封装|[librbtree](librbtree)|内核rbtree
|[librpc](librpc)|远程过程调用库|[librtsp](librtsp)|RTSP协议|[libskt](libskt)|Socket封装
|[libstun](libstun)|STUN协议库|[libtime](libtime)|时间库|[libqueue](libqueue)|数据队列
|[libringbuffer](libringbuffer)|循环缓冲|[libworkq](libworkq)|工作队列|[libvector](libvector) |容器库
|[libuvc](libuvc)|USB摄像头库

## 编译方法
推荐 Ubuntu14.04 gcc-4.8.4+ 及以上版本

  * linux 平台 (32/64 bit)  
   `$ cd libraries`  
   `$ ./build.sh`  
   `$ sudo ./build.sh install`

  * host(linux) target(rasberrypi)  
    (you need download [toolchain of rasberrypi](https://github.com/raspberrypi/tools.git))  
   `$ ./build.sh all pi`

  * host(rasberrypi board)  
   `$ ./build.sh`  
   `$ sudo ./build.sh install`  

  * android arm 交叉编译  
   (需要下载 [android-ndk-r9-linux-x86_64.tar.bz2](http://dl.google.com/android/ndk/android-ndk-r9-linux-x86_64.tar.bz2))  
   `$ ./build.sh all android`  

  * android 原生开发  
    also need ndk tools  
   `$ cd android_jni_libs`  
   `$ ndk-build`  

   After install, the libxxx.xx will be installed in /usr/local/lib/.  
   libxxx.h, libxxx.so or libxxx.a of libraries are also in ./output/$(ARCH)  

## 自动生成代码
   如果要加入你自己的开发库，只需要执行如下命令，即可自动生成代码和编译框架  
  `$ ./autogen_lib.sh libfoo`

## License
LGPL/GPLv3. Please refer to the LICENSE file for detailed information.

* 一般的开源项目如nginx/ffmpeg/redis等，都有各自的基础库，且实现较为相近，取各库的优点，实现较为通用的库，且库的命名不带特定工程前缀，在实现自己的工程时，方便代码的快速集成。
* 当库完成度和稳定性高时，会release 到ubuntu launchpad.net PPA供下载安装

* liblog 已提供ubuntu 16.04 64bit下载

## apt-get获取

  $ `sudo add-apt-repository ppa:gozfree/ppa`  
  $ `sudo apt-get update`  
  $ `sudo apt-get install liblog`


## 作者与贡献者
非常欢迎参与开发维护这套基础库

|                                               |                                               |
|-----------------------------------------------|-----------------------------------------------|
| [CMShuyuhui](https://github.com/CMShuyuhui)   | [core1011](https://github.com/core1011)       |
| [elfring](https://github.com/elfring)         | [ktsaou](https://github.com/ktsaou)           |
| [zh794390558](https://github.com/zh794390558) | [gozfree](https://github.com/gozfree)         |
