### 介绍

- 此程序功能为使用 `Rtmp` 协议对本地视频进行封装，以 `Rtmp` 流媒体的形式推送至 `Rtmp` 流媒体服务器发布。

- 此程序主要参考雷霄骅博士的 ["最简单的基于FFmpeg的推流器(推送RTMP)"](https://github.com/leixiaohua1020/simplest_ffmpeg_streamer) 项目编写，根据 `FFmpeg` 更新修改。

### 环境

- OS: win7/64-bit
- IDE: Visual Studio 2013
- FFmpeg: 3.2.2/32-bit, dev\&shared

### 功能说明

- 由于项目需求，需要对多个连续的视频进行循环播放
- 由于某些原因，视频无音频数据，所以程序中过滤了音频数据，不进行推送

### 程序运行配置

- `argv[1]`: `ini` 配置文件名称

### 配置文件说明

- ConfigInfo: 配置信息
    - `rtmp_url`: `rtmp` 视频流播放地址
    - `file_num`: 播放视频文件数量

- `File`: 视频文件信息
    - `1`: 视频文件序号
    - `E:\FFmpeg\test\testVideo.mp4`: 视频文件绝对路径

### FAQ

1. 从 `Debug` 更换为 `Release` 后运行报错 `error LNK2026: 模块对于 SAFESEH 映像是不安全的`。
    解决:
    1. 打开 ***项目属性*** 下的 ***配置属性***
    2. 在 ***链接器 -> 命令行*** 中, ***其他选项中*** 添加 `/SAFESEH:NO`
