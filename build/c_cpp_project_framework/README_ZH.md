C CPP Project Framework (Template)
===================

[English](./README.md)

一个足够 **简单易用** 并且 **可配置的**用于构建 `C/C++` 的模板工程
> 使用 `CMake` 构建，并且支持带`GUI`配置界面的 `Kconfig`

基于此工程，可以快速搭建你的项目构建系统，减少许多不必要的麻烦～

希望能帮到你，如果帮到了请右上角给颗小星星哦  ～～ 如果有啥不足欢迎提 [issue](https://github.com/Neutree/c_cpp_project_framework/issues/new)。  `(´ε｀ ʃƪ)♡`

本项目的目标人群：

* 正准备写 `SDK` 并且需要自己写构建系统的开发者
* 正准备开始编写一个需要写构建系统的开发者
* 对`CMake`不太了解，但想学习 `CMake` 的开发者
* 想重构代码工程构建系统的，比如因为之前写的构建系统太杂乱
* 想给构建系统中加一个十分好用的配置系统，可以按需求快速增删代码模块的，而且最好带界面配置的 开发者
* 想让项目可以生成多种 `IDE` 支持的工程


![](assets/image/build.gif)


## 特性

* 语法简单, 无需`Makefile`或者`CMake`知识, 只需基于模板修改变量的值即可非常简单地用起来了
* 基于组件(component)概念的项目结构, 方便搭建层次结构清晰的项目架构
* 组件之间调用只需要一个语句指定依赖即可(比如`list(APPEND ADD_REQUIREMENTS component1)`), 无需设置多余变量比如`INCLUDE 路径`即可在源码中调用依赖的组件的内容
* 使用 `Kconfig` 使项目组件和代码可裁剪配置, 方便项目满足不同的需求
* 方便地引入静态库(`.a`) 和 动态库(`.so`) ( 比如`list(APPEND ADD_STATIC_LIB "lib/libtest.a")`)
* 方便地生成静态库(`.a`) 和 动态库(`.so`) (默认生成静态库, 需要组件生成动态库,则使用`register_component(DYNAMIC)`注册模块即可)
* 使用 `Python` 脚本作为辅助, 可方便地添加命令和工具, 编译只需要执行简单的命令即可(如`python project.py build` `python project.py menuconfig`)
* 方便地作为 `SDK`, 工程实例可以直接放在`SDK`目录下, 也可以单独放在磁盘任何地方, 只需要设置环境变量`MY_SDK_PATH`即可
* 交叉编译友好, 很好地作为嵌入式设备 `SDK`


## 快速上手

* 克隆代码:
```
git clone https://github.com/Neutree/c_cpp_project_framework --recursive
```
> 参数 `--recursive` 是必须的, 是为了克隆子模块, 否则克隆下来的代码不完整

* 或者基于这个模板仓库创建一个你自己的 github 仓库:</br>点击 `use this template` 按钮即可</br>![](assets/image/use_template.png)

* 开始编译</br>有两种方法, 使用`project.py`脚本或者使用原生`CMake`命令
  * 使用`project.py`脚本
    ```
    cd examples/demo1
    # python project.py --toolchain /opt/toolchain/bin --toolchain-prefix mips-elf- config
    python project.py menuconfig
    python project.py build
    ./build/demo1
    python project.py clean
    python project.py distclean
    # python project.py clean_conf
    ```
    * 切换工程目录
    * 设置工具链路径以及前缀（如果使用`gcc`不需要设置）
    * 使用命令 `python project.py menuconfig` 来通过终端图形界面配置工程, 这会在 `build/config` 目录生成几个配置文件（`global_config.*`), 我们可以直接在组件(`component`)的`CMakelists.txt` 文件中直接使用（详细看后面的说明）， 或者在 `C/CPP`源文件中通过语句 `#include "global_config.h"` 包含配置头文件来使用
    * 使用命令 `python project.py build` 来启动构建, 可以使用`python project.py build --verbose`命令来打印编译时的调试信息
    * 使用`python project.py clean`来清除构建中间文件, 使用`python project.py distclean`来清楚`menuconfig`命令生成的文件, 这个命令不会清除工具链配置
    * 使用`python project.py clean_conf`来清除工具链配置
  * 使用原生`CMake`命令
    ```
    cd examples/demo1
    # python project.py --toolchain /opt/toolchain/bin --toolchain-prefix mips-elf- config
    mkdir build && cd build
    cmake ..
    make menuconfig
    make -j10
    ./build/demo1
    make clean
    rm -rf ./*
    ```
    * 切换工程目录
    * 设置工具链路径以及前缀（如果使用`gcc`不需要设置）
    * 建立一个临时目录并且切换当前路径到这个临时目录（`build`）
    * 使用命令 `cmake ..` 来生成 `Makefile`, 这里 `..` 表示上层目录，即项目的目录
    * 使用命令 `make menuconfig` 来通过终端图形界面配置工程, 这会在 `build/config` 目录生成几个配置文件（`global_config.*`), 我们可以直接在组件(`component`)的`CMakelists.txt` 文件中直接使用（详细看后面的说明）， 或者在 `C/CPP`源文件中通过语句 `#include "global_config.h"` 包含配置头文件来使用
    * 使用命令 `make` 来执行编译链接过程, 或者并行编译： [make -jN](http://www.gnu.org/software/make/manual/make.html#Parallel)， 以及通过 `make VERBOSE=1` 命令来打印编译时的调试信息


## 目录结构

| 目录/文件       | 功能 |
| -------------- | -------- |
| 根目录          | 本项目的根目录，也是 `SDK` 项目的 `SDK` 目录|
| assets         | 存放多媒体资源的文件夹，比如图片等，如果不需要可以删除 |
| components     | 组件(component)都放在这里 |
| examples       | 工程目录，或者例程目录；在 `SDK` 项目中这个目录是可以和 `SDK` 目录分开放的， 只需要设置环境变量`MY_SDK_PATH`为`SDK`目录路径即可 |
| tools          | 工具目录比如 `cmake`、`kconfig`、`burn tool` etc. |
| Kconfig        | `Kconfig` 的最顶层配置 |

### 1) 组件（component）

所有库均作为组件(component)被放在`components`目录下或者工程目录下，每个组件用一个目录，这个目录就是这个组件的名字， 为了使工程看起来更简洁，不对组件进行嵌套，所有组件都是一个层级，组件之间的关系依靠依赖关系（requirements）来维持

所有源文件都必须在某个组件内，每个工程必须包含一个叫 `main` 的组件（即`examples/demo1/main` 目录），每个组件包含文件如下：

* `CMakeLists.txt`： 必须有，声明组件源文件以及依赖的组件，并且调用注册函数注册自己，详细可以参考`components/component1`和`components/component2`下`CMakeLists.txt`的写法

* `Kconfig`： 可选，包含了本组件的配置选项， 在本组件或者其它依赖了本组件的组件的`CMakeLists.txt`中都可以在加一个`CONFIG_`前缀后使用这些配置项，比如在`components/component2`中，`Kconfig`中有`COMPONENT2_ENABLED` 选项，我们在它的`CMakeLists.txt`中就使用了这个变量`if(CONFIG_COMPONENT2_ENABLED)`来判断如果用户配置不用这个组件就不注册这个组件

### 2) 工程目录

工程目录在`examples`目录下，当然，这个目录的名字是可以随意根据实际需求修改的，下面可以包含多个实际的工程目录，需要编译那个工程时切换到对应的目录就可以编译。上面也说了，每个工程目录下必须有一个 `main` 组件， 当然也可以放很多自定义的组件。 可以参考`examples/demo1`工程目录。

工程目录下文件：

* `CMakeLists.txt`： 工程属性文件，必须先包含`include(${SDK_PATH}/tools/cmake/compile.cmake)`，然后用`project`函数声明工程名称，比如`project(demo1)`，当然还可以编写其它的条件或者变量等，使用`CMake`语法， 参考`examples/demo1/CMakeLists.txt`的写法

* `config_defaults.mk`： 工程默认配置文件，执行`cmake`构建时会从这里加载默认配置，配置的格式是`Makefile`的格式，可以先使用终端界面配置(`make menuconfig`)生成配置文件复制过来，生成的配置文件在`build/config/global_config.mk`。
> 注意：每次修改`config_defaults.mk` 后需要删除`build`目录下的文件(或者只删除`build/config/global_config.mk`文件)重新生成，因为当前构建系统会优先使用`build`目录下已经存在的配置文件

* `project.py`: 工具脚本调用入口, 使用`python project.py menuconfig` `python project.py build` 等命令来开始构建

如何将工程目录放在磁盘的任何地方: 

* 将`CMakeLists.txt`和 `project.py` 中的 `MY_SDK_PATH` 改成你喜欢的环境变量名称, 然后在终端中设置这个环境变量的值为`SDK`的路径, 即可将这个工程目录放到任何地方也可以编译了

## SDK 和 工程目录分开存放

通常情况下，只需要按照自己的需求，修改`example`目录的名字，比如改成`projects`，或者在工程根目录重新创建一个目录也是可以的，比如`projects/hello_world`，然后拷贝`examples/demo1`中的内容来新建一个新的工程

另外，工程目录和 SDK 目录也可以分开存放，这通常适用于开源项目，一份SDK，用户基于这份 SDK 开发，这样更利于源码传播，用户不需要拷贝一份 SDK， 只需要指定使用的 SDK 版本（git 提交号）。
要做到，只需要：

* 下载 `SDK` 放到某个目录，比如 `/home/neucrack/my_SDK`

```
git clone https://github.com/Neutree/c_cpp_project_framework --recursive
```
注意这里用了`--recursive`参数， 因为工程中使用了子模块，子模块的好处是各个工程分开管理，比如这里用了`Kconfiglib`作为子模块，提供`menuconfig`带界面的功能配置功能

**注意如果不更新子模块，将无法通过编译！！！**

如果克隆的时候忘记加这个参数了，也可以再使用下面这个命令来更新子模块：
```
git submodule update --init --recursive
```
另外，当远程仓库更新了，用户也需要使用以下命令来更新代码（即同时更新子模块代码）：
```shell
git pull --recursive
```
或者：
```
git pull
git submodule update --init --recursive
```

当然，你也可以选择删除 `.git` 目录，然后重新创建一个没有子模块的 git 仓库～

* 然后在终端导出变量 `export MY_SDK_PATH=/home/neucrack/my_SDK`， 可以放到 `~/.bashrc`或者`~/.zshrc`文件中，这样每次终端启动都会自动添加这个变量了
* 然后在任意地方建立工程， 比如拷贝`example/demo1`整个文件夹中的所有文件到`/home/neucrack/temp/my_projects/demo1`
* 然后清除之前的构建缓存（如果有的话，没有就忽略）
```
python3 project.py distclean
```
* 然后配置和构建即可
```
python3 project.py menuconfig
python3 project.py build
```

## 开源许可

**MIT**: 详情看 [LICENSE](./LICENSE) 文件

## 此工程使用到的开源项目

* [Kconfiglib](https://github.com/ulfalizer/Kconfiglib)： `Kconfig` `Python` 实现

## 使用了此框架的仓库

* [Maix-Speech](https://github.com/sipeed/Maix-Speech): 专为嵌入式环境设计的离线语音库
* [MaixPy](https://github.com/sipeed/MaixPy/): 让针对`AIOT`应用的芯片`K210`用上`Micropython`
* [libmaix](https://github.com/sipeed/libmaix): 支持多嵌入式平台的带硬件加速的 AI 模型运行库
* [MF1_SDK](https://github.com/sipeed/MF1_SDK): `MF1` AI 模组（开发板）的 SDK


## 相关类似参考项目

* [ESP_IDF](https://github.com/espressif/esp-idf)： `ESP32` 的 `SDK`， 写得挺不错
* [RT-Thread](https://github.com/RT-Thread/rt-thread)：不是用的 `CMake`, 但是也是使用了组件的概念

