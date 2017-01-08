//#pragma once
//#pragma execution_character_set("utf-8")

#include <iostream>
#include "streamer.h"
#include "vr_ini.h"

using namespace std;

#ifndef test
	#define test
#endif

int main(int argc, char* argv[])
{
	CIniFile ini_file;

	//ini_file.OpenIniFile("videoConfig.ini");
	ini_file.OpenIniFile(argv[1]);

	char *strBasicParam;
	char *strTemp;

	const char *temp;
	int temp_len;
	int file_num;
	char **files;
	char *rtmp_url;
	int i;

#ifdef test

	std::cout << "*** Get system infomation ......" << endl;
	strBasicParam = "ConfigInfo";
	strTemp = "file_num";
	file_num = ini_file.ReadInt(strBasicParam, strTemp, 1);
	std::cout << "*** file num : " << file_num << endl;

	files = new char*[file_num];

	strTemp = "rtmp_url";
	temp = ini_file.ReadString(strBasicParam, strTemp, "0");
	temp_len = strlen(temp);
	rtmp_url = new char[temp_len + 1];
	std::memset(rtmp_url, 0, temp_len + 1);
	std::memcpy(rtmp_url, temp, temp_len);
	*(rtmp_url + temp_len + 1) = '\0';

	std::cout << "*** rtmp url : " << rtmp_url << endl;

	strBasicParam = "File";
	strTemp = new char[3];
	for (i = 1; i <= file_num; ++i)
	{
		sprintf(strTemp, "%d", i);
		std::cout << "strTemp: " << strTemp << endl; 

		temp = ini_file.ReadString(strBasicParam, strTemp, "0");
		temp_len = strlen(temp);
		*(files + i - 1) = new char[temp_len + 1];
		std::memset(*(files + i - 1), 0, temp_len + 1);
		std::memcpy(*(files + i - 1), temp, temp_len);
		*(*(files + i - 1) + temp_len + 1) = '\0';
		std::cout << "file " << i << ": " << *(files + i - 1) << files + i - 1 << endl;
	}

	delete strTemp;
	ini_file.CloseIniFile();

	Streamer((const char **)files, file_num, rtmp_url);

	//Streamer("8S039宽北巷社区青海寺小区大门14.mp4", "rtmp://10.10.2.223/live/14 live=1");

	//Streamer("8S039宽北巷社区青海寺小区大门12.flv", "rtmp://10.10.2.223/live/test live=1");
	//Streamer("cuc_ieschool.flv", "rtmp://10.10.2.223/live/test live=1");
	//Streamer("8S039宽北巷社区青海寺小区大门_12.mp4", "rtmp://10.10.2.223/live/test live=1");

#else

	strBasicParam = "ConfigInfo";
	strTemp = "file_num";
	file_num = ini_file.ReadInt(strBasicParam, strTemp, 1);

	files = new const char*[file_num];

	strTemp = "rtmp_url";
	rtmp_url = ini_file.ReadString(strBasicParam, strTemp, "0");

	strBasicParam = "File";
	strTemp = new char[3];
	for (i = 1; i <= file_num; i++)
	{
		sprintf(strTemp, "%d", i);
		*(files + i - 1) = ini_file.ReadString(strBasicParam, strTemp, "0");
	}

	ini_file.CloseIniFile();

#endif

	delete[] files;

	return 0;
}
