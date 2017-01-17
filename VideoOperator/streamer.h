//#pragma once
//#pragma execution_character_set("utf-8")

#ifndef ffmpeg_include

#define ffmpeg_include

#define __STDC_CONSTANT_MACROS

/**
* C++ 中使用 C语言函数库
* 需要添加 extern "C"
*/
extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\time.h"
#include "libavutil\log.h"
}

#endif

int Streamer(const char **files, int file_num, const char *out_filename);

int testStreamer(const char **files, int file_num, const char *out_filename);