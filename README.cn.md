libraries [English](README.md) [![Build Status](https://travis-ci.org/gozfree/libraries.svg?branch=master)](https://travis-ci.org/gozfree/libraries)
=========

libraries是一组通用的Ｃ基础库，目标是为减少重复造轮子而写。

* 全部用POSIX C实现，目标是为了跨平台兼容x86, arm, android, ios.
* 每个库尽可能各自独立，而且ＡＰＩ容易使用
* 尽量不依赖任何第三方库
* 目标是为减少重复造轮子
* 实现了日志、原子操作、哈希字典、红黑树、动态库加载、线程、锁操作、配置文件、ｏｓ适配层、事件驱动、工作队列、ＲＰＣ、ＩＰＣ等基础库，和p2p穿透等网络库
* 一般的开源项目如nginx/ffmpeg/redis等，都有各自的基础库，且实现较为相近，取各库的优点，实现较为通用的库，且库的命名不带特定工程前缀，在实现自己的工程时，方便代码的快速集成。
* 当库完成度和稳定性高时，会release 到ubuntu launchpad.net PPA供下载安装

* liblog 已提供ubuntu 16.04 64bit下载
##How To apt-get
  $ `sudo add-apt-repository ppa:gozfree/ppa`
  $ `sudo apt-get update`
  $ `sudo apt-get install liblog`
