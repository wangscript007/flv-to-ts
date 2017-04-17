#pragma once

#define AAC_Main 1
#define AAC_LC 2
#define AAC_SSR 3
#define AAC_LTP 4
#define AAC_Scalable 6;
#define AAC_TwinVQ 7

#include "RTP.h"

struct AudioSC
{
	char audioObjectType;	// 1 main 2 lc 3 ssr
	char samplingFrequencyIndex;
	int samplingFrequency;
	char channelConfiguration;
};

struct ADTS
{
	//1
	char ID;							//1 1:MPEG2 0:MPEG4 一般用MPEG4
	char Layer;							//2 固定0
	char protection_absent;				//1 error_check --1:没有错误检查  0:有错误检查
	//2
	char Profile_ObjectType;			//2 MPEG2：{main:0 lc:1 ssr:2} MPEG4{main:0 lc:1 ssr:2}
	char sampling_frequency_index;		//4 同ASC
	char private_bit;					//1 0
	char channel_configuration;			//3 同ASC
	//3
	char original_copy;					//1 0
	char home;							//1 0
	//char Emphasis;						//2 0

	char copyright_identification_bit;	//1 0
	char copyright_identification_start;//1 0
	short aac_frame_length;				//13 包括头在内的总长度
	short adts_buffer_fullness;			//11 0x7ff
	char no_raw_data_blocks_in_frame;	//2 0
};

void InitAudioSpecificConfig(unsigned char *ascData,AudioSC& ascOut);
void generateADTSHeader(AudioSC& asc, int frameLength, DataPacket &adts);