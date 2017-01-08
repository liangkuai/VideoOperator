//#pragma once
//#pragma execution_character_set("utf-8")

#define __STDC_CONSTANT_MACROS

#ifndef test
	#define test
#endif

/**
* C++ 中使用 C语言函数库
* 需要添加 extern "C"
*/
extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\time.h"
}

int Streamer(const char **files, int file_num, const char *out_filename)
{
	AVFormatContext *in_fmt_ctx = NULL;
	AVFormatContext *out_fmt_ctx = NULL;

	int tns, thh, tmm, tss;

	int file_order = 0;

	int ret;
	int videoindex = -1;
	int i;

	AVOutputFormat *ofmt = NULL;

	int64_t start_time = 0;
	AVPacket pkt;
	int64_t frame_index = 0;

#ifdef test
	//in_filename = "cuc_ieschool.flv";		// 输入URL（Input file URL）
	for (i = 0; i < file_num; ++i)
		printf("------Input %d: %s\n", i, *(files+i));

	//out_filename = "rtmp://10.10.2.223/live/test live=1";		// 输出 URL（Output URL）[RTMP]
	printf("------Output: %s\n", out_filename);
#endif

	// 注册入口函数，注册编解码器
	av_register_all();

	// Network
	avformat_network_init();

	// 打开输入流，初始化 Input_AVFormatContext
	if ((ret = avformat_open_input(&in_fmt_ctx, *(files + file_order), NULL, NULL)) < 0)
	{
		printf("Could not open input file.");
		goto end;
	}

	/**
	* avformat_find_stream_info: 获取一部分视频数据和相关信息
	* 执行正常: return >= 0
	*/
	if ((ret = avformat_find_stream_info(in_fmt_ctx, 0)) < 0)
	{
		printf("Failed to retrieve input stream information");
		goto end;
	}

	// 指出 Video 所属 AVStream 下标
	for (i = 0; i < in_fmt_ctx->nb_streams; i++)
	{
		if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			videoindex = i;
			break;
		}
	}

	// dump 输入流信息到 AVFormatContext 中
	av_dump_format(in_fmt_ctx, 0, *(files + file_order), 0);

#ifdef test
	printf("------in_fmt_ctx nb_streams: %d\n", in_fmt_ctx->nb_streams);
	for (i = 0; i < in_fmt_ctx->nb_streams; i++)
	{
		tns = in_fmt_ctx->streams[i]->duration / 1000000;
		thh = tns / 3600;
		tmm = (tns % 3600) / 60;
		tss = (tns % 60);

		printf("------ %d frame rate: %d/%d, codec type: %d, time_base: %d/%d, duration: %02d:%02d:%02d, flags: %d\n", i,
			in_fmt_ctx->streams[i]->avg_frame_rate.num,
			in_fmt_ctx->streams[i]->avg_frame_rate.den,
			in_fmt_ctx->streams[i]->codecpar->codec_type,
			in_fmt_ctx->streams[i]->time_base.num,
			in_fmt_ctx->streams[i]->time_base.den,
			thh, tmm, tss,
			in_fmt_ctx->flags);
	}
#endif

	// 初始化 Output_AVFormatContext
	avformat_alloc_output_context2(&out_fmt_ctx, NULL, "flv", out_filename); //RTMP
	if (!out_fmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	ofmt = out_fmt_ctx->oformat;
	for (i = 0; i < in_fmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream *in_stream = in_fmt_ctx->streams[i];
		//AVStream *out_stream = avformat_new_stream(out_fmt_ctx, in_stream->codec->codec);
		AVStream *out_stream = NULL;
		if (i == 0)
			out_stream = avformat_new_stream(out_fmt_ctx, in_fmt_ctx->video_codec);
		else
			out_stream = avformat_new_stream(out_fmt_ctx, in_fmt_ctx->audio_codec);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

#ifndef test
		//Copy the settings of AVCodecContext
		//ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		ret = avcodec_parameters_to_context(out_stream->codec, in_stream->codecpar);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codec->codec_tag = 0;
		if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
#else
		ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			goto end;
		}
		out_stream->codecpar->codec_tag = 0;
		if (out_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			//out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
			out_fmt_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
#endif

		if (i == 0)
			break;
	}

	// dump 输出流信息到 AVFormatContext 中
	av_dump_format(out_fmt_ctx, 0, out_filename, 1);

	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			goto end;
		}
	}

	//Write file header
	ret = avformat_write_header(out_fmt_ctx, NULL);
	//ret = avformat_init_output(out_fmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		goto end;
	}

	start_time = av_gettime();

	while (1) {
		AVStream *in_stream, *out_stream;

		//Get an AVPacket
		ret = av_read_frame(in_fmt_ctx, &pkt);
		if (ret < 0)
		{
			//frame_index = 0;
			ret = 0;

			if (file_order == file_num - 1)
				file_order = 0;
			else
				++file_order;

			av_packet_unref(&pkt);

			avformat_free_context(in_fmt_ctx);
			in_fmt_ctx = NULL;

			// 打开输入流，初始化 Input_AVFormatContext
			//if ((ret = avformat_open_input(&in_fmt_ctx, "8S039宽北巷社区青海寺小区大门_12.mp4", NULL, NULL)) < 0)
			if ((ret = avformat_open_input(&in_fmt_ctx, *(files + file_order), NULL, NULL)) < 0)
			{
				printf("Could not open input file.");
				goto end;
			}

			/**
			 * avformat_find_stream_info: 获取一部分视频数据和相关信息
			 * 执行正常: return >= 0
			 */
			if ((ret = avformat_find_stream_info(in_fmt_ctx, 0)) < 0)
			{
				printf("Failed to retrieve input stream information");
				goto end;
			}

			// 指出 Video 所属 AVStream 下标
			for (i = 0; i < in_fmt_ctx->nb_streams; i++)
			{
				if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
					videoindex = i;
					break;
				}
			}

			// dump 输入流信息到 AVFormatContext 中
			//av_dump_format(in_fmt_ctx, 0, "8S039宽北巷社区青海寺小区大门_12.mp4", 0);
			av_dump_format(in_fmt_ctx, 0, *(files + file_order), 0);

			printf("Push over 1");
			continue;
		}

//#ifndef test
		//FIX：No PTS (Example: Raw H.264)
		//Simple Write PTS
		//if (pkt.pts == AV_NOPTS_VALUE){
			//Write PTS
			AVRational time_base1 = in_fmt_ctx->streams[videoindex]->time_base;
			//Duration between 2 frames (us)
			int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_fmt_ctx->streams[videoindex]->r_frame_rate);
			//Parameters
			pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
			pkt.dts = pkt.pts;
			pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		//}
//#endif

		//Important:Delay
		if (pkt.stream_index == videoindex){
			AVRational time_base = in_fmt_ctx->streams[videoindex]->time_base;
			AVRational time_base_q = { 1, AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);


			in_stream = in_fmt_ctx->streams[pkt.stream_index];
			out_stream = out_fmt_ctx->streams[pkt.stream_index];

			/* copy packet */
			//Convert PTS/DTS
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;
		}

		


		//Print to Screen
		if (pkt.stream_index == videoindex){
			printf("Send %8d video frames to output URL\n", frame_index);
			frame_index++;

			//ret = av_write_frame(out_fmt_ctx, &pkt);
			ret = av_interleaved_write_frame(out_fmt_ctx, &pkt);
		}

		


		if (ret < 0) {
			printf("Push over 2");
			printf("Error muxing packet\n");
			break;
		}

		av_packet_unref(&pkt);
		//av_free_packet(&pkt);
	}

	//Write file trailer
	if ((ret = av_write_trailer(out_fmt_ctx)) == 0)
	{
		printf("Push over 3");
	}


end:
	avformat_close_input(&in_fmt_ctx);
	/* close output */
	if (out_fmt_ctx && !(out_fmt_ctx->flags & AVFMT_NOFILE))
		avio_close(out_fmt_ctx->pb);
	avformat_free_context(out_fmt_ctx);
	if (ret < 0 && ret != AVERROR_EOF) {
		printf("Error occurred.\n");
		return -1;
	}

	return 0;
}