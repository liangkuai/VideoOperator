//#pragma once
//#pragma execution_character_set("utf-8")

#include "decode_operator.h"

int input_operator(AVFormatContext **in_fmt_ctx, const char **files, int file_order, AVDictionary *options, int *videoindex)
{
	int ret;
	int i;

	// ��ʼ������
	// 1. �����벢��ȡ�����Ϣ -> ��ʼ�� in_fmt_ctx
	// 2. ��ȡһ��������Ƶ���ݲ���ȡ�����Ϣ
	if ((ret = avformat_open_input(in_fmt_ctx, *(files + file_order), NULL, &options)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Counld not open input file");
		goto end;
	}
	if ((ret = avformat_find_stream_info(*in_fmt_ctx, 0)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information");
		goto end;
	}

	av_log(NULL, AV_LOG_INFO, "streams number : %d\n", (*in_fmt_ctx)->nb_streams);
	av_log(NULL, AV_LOG_INFO, "start time : %ld\n", (*in_fmt_ctx)->start_time);
	av_log(NULL, AV_LOG_INFO, "duration : %lld\n\n", (*in_fmt_ctx)->duration);

	// ָ�� Video ���� AVStream �±�
	for (i = 0; i < (*in_fmt_ctx)->nb_streams; i++)
	{
		if ((*in_fmt_ctx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			*videoindex = i;
			av_log(NULL, AV_LOG_INFO, "codec type : video\n");
		}
		else if ((*in_fmt_ctx)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			av_log(NULL, AV_LOG_INFO, "codec type : audio\n");
		}
		else
		{
			av_log(NULL, AV_LOG_INFO, "codec type : other\n");
		}
		av_log(NULL, AV_LOG_INFO, "time base: %d/%d\n", (*in_fmt_ctx)->streams[i]->time_base.num, (*in_fmt_ctx)->streams[i]->time_base.den);
		av_log(NULL, AV_LOG_INFO, "avg_frame_rate: %d/%d\n", (*in_fmt_ctx)->streams[i]->avg_frame_rate);
		printf("\n");
	}

	// dump ��������Ϣ�� AVFormatContext ��
	av_dump_format((*in_fmt_ctx), 0, *(files + file_order), 0);

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