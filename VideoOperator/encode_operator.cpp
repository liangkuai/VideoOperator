//#pragma once
//#pragma execution_character_set("utf-8")

#include "encode_operator.h"

int output_operator(AVFormatContext **in_fmt_ctx, AVFormatContext **out_fmt_ctx, const char *out_filename, int videoindex)
{
	int ret;
	int i;

	// 初始化输出
	// 初始化 out_fmt_ctx
	avformat_alloc_output_context2(out_fmt_ctx, NULL, "flv", out_filename); //RTMP
	if (!out_fmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	// 1. 根据输入流创建输出流
	// 2. 复制输入流信息至输出流
	// 由于视频原因，缺失音频信息；此处不为输出创建音频流
	for (i = 0; i < (*in_fmt_ctx)->nb_streams; i++) {
		AVStream *in_stream = (*in_fmt_ctx)->streams[i];

		// @deprecated
		// AVStream *out_stream = avformat_new_stream(out_fmt_ctx, in_stream->codec->codec);

		AVStream *out_stream = NULL;
		if (i == videoindex)
			out_stream = avformat_new_stream(*out_fmt_ctx, (*in_fmt_ctx)->video_codec);
		if (!out_stream) {
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codecpar->codec_tag = 0;
		if ((*out_fmt_ctx)->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		//out_fmt_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

		if (i == 0)
			break;
	}

	// dump 输出流信息到 AVFormatContext 中
	av_dump_format((*out_fmt_ctx), 0, out_filename, 1);

	return ret;

end:
	av_log(NULL, AV_LOG_ERROR, "*** END");
	avformat_close_input(in_fmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF)
	{
		av_log(NULL, AV_LOG_ERROR, "Error occurred.\n");
	}
	return ret;
}