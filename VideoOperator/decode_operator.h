//#pragma once
//#pragma execution_character_set("utf-8")


#ifndef ffmpeg_include

#define ffmpeg_include

#define __STDC_CONSTANT_MACROS

/**
* C++ ��ʹ�� C���Ժ�����
* ��Ҫ��� extern "C"
*/
extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\time.h"
#include "libavutil\log.h"
}

#endif

int input_operator(AVFormatContext *in_fmt_ctx, const char **files, int file_order, AVDictionary *options, int *videoindex);