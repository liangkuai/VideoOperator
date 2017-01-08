//#pragma once
//#pragma execution_character_set("utf-8")

#include "Tool.h"
#include <stdint.h>
#include <iostream>
#include <queue>

using namespace std;



/**
* AVFormatContext->duration ������ʱ��ת��
* duration����΢��(us)Ϊ��λ
* ת����hh:mm:ss��ʽ
*/
int durationConvert(int64_t duration)
{
	int tns, thh, tmm, tss;
	tns = duration / 1000000;
	thh = tns / 3600;
	tmm = (tns % 3600) / 60;
	tss = (tns % 60);
	cout << thh << ":" << tmm << ":" << tss;

	return 0;
}