/**
 * Project name: VideoOperator
 * 
 * Push rtmp stream from local video to rtmp-server by FFmpeg 
 * 
 * Create by LiuKai
 */

//#pragma once
//#pragma execution_character_set("utf-8")

#include <iostream>
#include "streamer.h"
#include "vr_ini.h"

#define __STDC_CONSTANT_MACROS
extern "C"
{
#include "libavutil\log.h"
}

using namespace std;

#ifndef test
	#define test
#endif

int main(int argc, char* argv[])
{
	CIniFile ini_file;		// object of system config file
	
	ini_file.OpenIniFile(argv[1]);

	char *strBasicParam;
	char *strTemp;

	const char *temp;
	int temp_len;
	int file_num;			// numbers of video files
	char **files;				// name of video files
	char *rtmp_url;		// rtmp url
	int i;

#ifdef test

	av_log(NULL, AV_LOG_INFO, "*** Openning system file information ......\n");
	strBasicParam = "ConfigInfo";
	strTemp = "file_num";
	file_num = ini_file.ReadInt(strBasicParam, strTemp, 1);
	av_log(NULL, AV_LOG_INFO, "*** Files number is: %d\n", file_num);

	files = new char*[file_num];

	strTemp = "rtmp_url";
	temp = ini_file.ReadString(strBasicParam, strTemp, "0");
	temp_len = strlen(temp);
	rtmp_url = new char[temp_len + 1];
	std::memset(rtmp_url, 0, temp_len + 1);
	std::memcpy(rtmp_url, temp, temp_len);
	*(rtmp_url + temp_len + 1) = '\0';

	av_log(NULL, AV_LOG_INFO, "*** Rtmp url is: %s\n", rtmp_url);

	strBasicParam = "File";
	strTemp = new char[3];
	for (i = 1; i <= file_num; ++i)
	{
		sprintf(strTemp, "%d", i);

		temp = ini_file.ReadString(strBasicParam, strTemp, "0");
		temp_len = strlen(temp);
		*(files + i - 1) = new char[temp_len + 1];
		std::memset(*(files + i - 1), 0, temp_len + 1);
		std::memcpy(*(files + i - 1), temp, temp_len);
		*(*(files + i - 1) + temp_len + 1) = '\0';

		av_log(NULL, AV_LOG_INFO, "file %d: %s\n", i, *(files + i - 1));
	}

	delete strTemp;
	ini_file.CloseIniFile();

	testStreamer((const char **)files, file_num, rtmp_url);
	//Streamer((const char **)files, file_num, rtmp_url);

	//Streamer("8S039宽北巷社区青海寺小区大门14.mp4", "rtmp://10.10.2.223/live/14 live=1");

	//Streamer("8S039宽北巷社区青海寺小区大门12.flv", "rtmp://10.10.2.223/live/test live=1");
	//Streamer("cuc_ieschool.flv", "rtmp://10.10.2.223/live/test live=1");
	//Streamer("8S039宽北巷社区青海寺小区大门_12.mp4", "rtmp://10.10.2.223/live/test live=1");

#else

	strBasicParam = "ConfigInfo";
	strTemp = "file_num";
	file_num = ini_file.ReadInt(strBasicParam, strTemp, 1);

	files = new char*[file_num];

	strTemp = "rtmp_url";
	temp = ini_file.ReadString(strBasicParam, strTemp, "0");
	temp_len = strlen(temp);
	rtmp_url = new char[temp_len + 1];
	std::memset(rtmp_url, 0, temp_len + 1);
	std::memcpy(rtmp_url, temp, temp_len);
	*(rtmp_url + temp_len + 1) = '\0';

	strBasicParam = "File";
	strTemp = new char[3];
	for (i = 1; i <= file_num; ++i)
	{
		sprintf(strTemp, "%d", i);

		temp = ini_file.ReadString(strBasicParam, strTemp, "0");
		temp_len = strlen(temp);
		*(files + i - 1) = new char[temp_len + 1];
		std::memset(*(files + i - 1), 0, temp_len + 1);
		std::memcpy(*(files + i - 1), temp, temp_len);
		*(*(files + i - 1) + temp_len + 1) = '\0';
}

	delete strTemp;
	ini_file.CloseIniFile();

	Streamer((const char **)files, file_num, rtmp_url);

#endif

	delete[] files;

	return 0;
}
