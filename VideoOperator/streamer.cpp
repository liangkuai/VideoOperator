//#pragma once
//#pragma execution_character_set("utf-8")

#include "streamer.h"
#include "decode_operator.h"
#include "encode_operator.h"

#ifndef test
	#define test
#endif

int testStreamer(const char **files, int file_num, const char *out_filename)
{
	AVFormatContext *in_fmt_ctx = NULL;
	AVFormatContext *out_fmt_ctx = NULL;

	int file_order = 0;
	int ret;
	int videoindex = -1;
	int i;
	int64_t start_time = 0;

	AVOutputFormat *ofmt = NULL;
	AVPacket pkt;
	int64_t frame_index = 0;

	// ��ʼ��
	// 1. ע����ں��� -> ע��������
	// 2. ��ʼ������
	av_register_all();
	ret = avformat_network_init();
	
	// TODO: options
	AVDictionary *options = NULL;
	//av_dict_set(&options, "video_size", "320x240", 0);

	// ��Ƶ����
	ret = input_operator(&in_fmt_ctx, files, file_order, options, &videoindex);
	if (ret < 0)
	{
		goto end;
	}

	// ��Ƶ����
	ret = output_operator(&in_fmt_ctx, &out_fmt_ctx, out_filename, videoindex);
	if (ret < 0)
	{
		goto end;
	}

	ofmt = out_fmt_ctx->oformat;
	// �����
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Could not open output URL '%s'");
			goto end;
		}
	}

	// д����Ƶ�ļ�ͷ��
	//ret = avformat_init_output(out_fmt_ctx, NULL);
	ret = avformat_write_header(out_fmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output URL\n");
		goto end;
	}

	start_time = av_gettime();

	while (1) {
		AVStream *in_stream, *out_stream;

		// ��ȡһ֡
		ret = av_read_frame(in_fmt_ctx, &pkt);
		if (ret < 0)
		{
			// ȡ֡ʧ��
			// 1. ��ǰ��Ƶ��ȡ��
			// 2. ȡ֡����
			// 
			// ����
			// 1. ��� AVPacket ����
			// 2. ����ִ��״̬��ʶ ret ��Ϊ 0
			// 3. ��ȡ��Ƶ�ļ���ʶѭ������һ���ƶ�
			// 4. �ͷŵ�ǰ Input AVFormatContext (in_fmt_ctx) ����
			// 5. ���³�ʼ�� Input AVFormatContext (in_fmt_ctx)
			av_packet_unref(&pkt);
			ret = 0;

			if (file_order == file_num - 1)
				file_order = 0;
			else
				++file_order;

			avformat_close_input(&in_fmt_ctx);
			in_fmt_ctx = NULL;

			// ��Ƶ����
			ret = input_operator(&in_fmt_ctx, files, file_order, options, &videoindex);
			if (ret < 0)
			{
				goto end;
			}

			av_log(NULL, AV_LOG_ERROR, "Push over, file order is: %d", file_order);
			continue;
		}

		// TODO: No PTS (Example: Raw H.264)
		// Simple Write PTS
		// Write PTS
		AVRational time_base1 = in_fmt_ctx->streams[videoindex]->time_base;
		/* Duration between 2 frames (us) */
		int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_fmt_ctx->streams[videoindex]->r_frame_rate);
		/* Parameters */
		pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
		pkt.dts = pkt.pts;
		pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);

		/* Important:Delay */
		if (pkt.stream_index == videoindex){
			AVRational time_base = in_fmt_ctx->streams[videoindex]->time_base;
			AVRational time_base_q = { 1, AV_TIME_BASE };
			int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
			int64_t now_time = av_gettime() - start_time;
			if (pts_time > now_time)
				av_usleep(pts_time - now_time);

			in_stream = in_fmt_ctx->streams[pkt.stream_index];
			out_stream = out_fmt_ctx->streams[pkt.stream_index];

			/* convert PTS/DTS and copy packet */
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;

			/* Print to screen */
			av_log(NULL, AV_LOG_INFO, "Send %8d video frames to output URL\n", frame_index);
			frame_index++;

			// ret = av_write_frame(out_fmt_ctx, &pkt);
			ret = av_interleaved_write_frame(out_fmt_ctx, &pkt);
		}

		// @deprecated
		//	av_free_packet(&pkt);
		av_packet_unref(&pkt);

		if (ret < 0) {
			av_log(NULL, AV_LOG_INFO, "Push over 2, ret: %d\n", ret);
			av_log(NULL, AV_LOG_INFO, "Error muxing packet\n");

			av_packet_unref(&pkt);
			ret = av_write_trailer(out_fmt_ctx);

			ret = 0;
		
			if (file_order == file_num - 1)
				file_order = 0;
			else
				++file_order;

			/* close input and output */
			avformat_close_input(&in_fmt_ctx);
			in_fmt_ctx = NULL;
			if (out_fmt_ctx && !(out_fmt_ctx->flags & AVFMT_NOFILE))
				avio_close(out_fmt_ctx->pb);
			avformat_free_context(out_fmt_ctx);
			out_fmt_ctx = NULL;

			// ��Ƶ����
			ret = input_operator(&in_fmt_ctx, files, file_order, options, &videoindex);
			if (ret < 0)
			{
				goto end;
			}
			
			// ��Ƶ����
			ret = output_operator(&in_fmt_ctx, &out_fmt_ctx, out_filename, videoindex);
			if (ret < 0)
			{
				goto end;
			}

			ofmt = out_fmt_ctx->oformat;
			// �����
			if (!(ofmt->flags & AVFMT_NOFILE)) {
				ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
				if (ret < 0) {
					av_log(NULL, AV_LOG_ERROR, "Could not open output URL '%s'");
					goto end;
				}
			}

			// д����Ƶ�ļ�ͷ��
			//ret = avformat_init_output(out_fmt_ctx, NULL);
			ret = avformat_write_header(out_fmt_ctx, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output URL\n");
				goto end;
			}

			start_time = av_gettime();

			continue;
		}

	}

	return 0;

end:
	avformat_network_deinit();
	avformat_close_input(&in_fmt_ctx);

	/* close output */
	if (out_fmt_ctx && !(out_fmt_ctx->flags & AVFMT_NOFILE))
		avio_close(out_fmt_ctx->pb);
	avformat_free_context(out_fmt_ctx);

	if (ret < 0 && ret != AVERROR_EOF)
	{
		av_log(NULL, AV_LOG_ERROR, "Error occurred.\n");
		return -1;
	}
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
	//in_filename = "cuc_ieschool.flv";		// ����URL��Input file URL��
	for (i = 0; i < file_num; ++i)
		printf("------Input %d: %s\n", i, *(files+i));

	//out_filename = "rtmp://10.10.2.223/live/test live=1";		// ��� URL��Output URL��[RTMP]
	printf("------Output: %s\n", out_filename);
#endif

	// ע����ں�����ע��������
	av_register_all();

	// Network
	avformat_network_init();

	AVDictionary *options = NULL;
	av_dict_set(&options, "video_size", "1600x1200", 0);
	//av_dict_set(&options, "pixel_format", "rgb24", 0);

	// ������������ʼ�� Input_AVFormatContext
	if ((ret = avformat_open_input(&in_fmt_ctx, *(files + file_order), NULL, &options)) < 0)
	{
		printf("Could not open input file.");
		goto end;
	}

	/**
	 * avformat_find_stream_info: ��ȡһ������Ƶ���ݺ������Ϣ
	 * ִ������: return >= 0
	 */
	if ((ret = avformat_find_stream_info(in_fmt_ctx, 0)) < 0)
	{
		printf("Failed to retrieve input stream information");
		goto end;
	}

	// ָ�� Video ���� AVStream �±�
	for (i = 0; i < in_fmt_ctx->nb_streams; i++)
	{
		if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
			videoindex = i;
			break;
		}
	}

	// dump ��������Ϣ�� AVFormatContext ��
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

	// ��ʼ�� Output_AVFormatContext
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
		AVStream *out_stream = NULL;
		//AVStream *out_stream = avformat_new_stream(out_fmt_ctx, in_stream->codec->codec);
		if (i == videoindex)
			out_stream = avformat_new_stream(out_fmt_ctx, in_fmt_ctx->video_codec);
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

		// ��Ƶ����Ƶ��Ϣ�򲻴�����Ƶ��
		if (i == 0)
			break;
	}

	// dump �������Ϣ�� AVFormatContext ��
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

			// ������������ʼ�� Input_AVFormatContext
			//if ((ret = avformat_open_input(&in_fmt_ctx, "8S039���������ຣ��С������_12.mp4", NULL, NULL)) < 0)
			if ((ret = avformat_open_input(&in_fmt_ctx, *(files + file_order), NULL, NULL)) < 0)
			{
				printf("Could not open input file.");
				goto end;
			}

			/**
			 * avformat_find_stream_info: ��ȡһ������Ƶ���ݺ������Ϣ
			 * ִ������: return >= 0
			 */
			if ((ret = avformat_find_stream_info(in_fmt_ctx, 0)) < 0)
			{
				printf("Failed to retrieve input stream information");
				goto end;
			}

			// ָ�� Video ���� AVStream �±�
			for (i = 0; i < in_fmt_ctx->nb_streams; i++)
			{
				if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
					videoindex = i;
					break;
				}
			}

			// dump ��������Ϣ�� AVFormatContext ��
			//av_dump_format(in_fmt_ctx, 0, "8S039���������ຣ��С������_12.mp4", 0);
			av_dump_format(in_fmt_ctx, 0, *(files + file_order), 0);

			printf("Push over 1");
			continue;
		}

//#ifndef test
		//FIX��No PTS (Example: Raw H.264)
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

			ret = av_write_frame(out_fmt_ctx, &pkt);
			//ret = av_interleaved_write_frame(out_fmt_ctx, &pkt);
		}

		
		av_packet_unref(&pkt);
		//av_free_packet(&pkt);

		if (ret < 0) {
			printf("Push over 2, ret: %d\n", ret);
			printf("Error muxing packet\n");
			//break;
			continue;
		}

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

	if (ret < 0 && ret != AVERROR_EOF)
	{
		av_log(NULL, AV_LOG_ERROR, "Error occurred.\n");
		return -1;
	}

	return 0;
}