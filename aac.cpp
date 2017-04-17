#include "aac.h"

void InitAudioSpecificConfig(unsigned char * ascData, AudioSC & ascOut)
{
	int cur = 0;
	ascOut.audioObjectType = (ascData[0] & 0xf8) >> 3;//5bit
	ascOut.samplingFrequencyIndex = ((ascData[0] & 0x7) << 1) | (ascData[1] >> 7); //3bit + 1bit
	if (ascOut.samplingFrequencyIndex==0xf)
	{
		//24bit
		//取频率
		int fre0, fre1, fre2;
		fre0 = ((ascData[1] & 0x7f) << 1) | (ascData[2] >> 7);
		fre1 = ((ascData[2] & 0x7f) << 1) | (ascData[3] >> 7);
		fre2 = ((ascData[3] & 0x7f) << 1) | (ascData[4] >> 7);
		ascOut.samplingFrequency = (fre0 << 16) | (fre1 << 8) | (fre2);
		cur = 4;
	}
	else
	{
		cur = 1;
		//查表得到频率
		switch (ascOut.samplingFrequencyIndex)
		{
		case 0:
			ascOut.samplingFrequency = 96000;
			break;
		case 1:
			ascOut.samplingFrequency = 88200;
			break;
		case 2:
			ascOut.samplingFrequency = 64000;
			break;
		case 3:
			ascOut.samplingFrequency = 48000;
			break;
		case 4:
			ascOut.samplingFrequency = 44100;
			break;
		case 5:
			ascOut.samplingFrequency = 32000;
			break;
		case 6:
			ascOut.samplingFrequency = 24000;
			break;
		case 7:
			ascOut.samplingFrequency = 22050;
			break;
		case 8:
			ascOut.samplingFrequency = 16000;
			break;
		case 9:
			ascOut.samplingFrequency = 12000;
			break;
		case 0xa:
			ascOut.samplingFrequency = 11025;
			break;
		case 0xb:
			ascOut.samplingFrequency = 8000;
			break;
		case 0xc:
			ascOut.samplingFrequency = 7350;
			break;
		default:
			ascOut.samplingFrequency = 44100;
			break;
		}
	}

	ascOut.channelConfiguration = (ascData[cur] & 0x78) >> 3;
	return;
}

void generateADTSHeader(AudioSC & asc, int frameLength, DataPacket & adts)
{
	adts.size = 7;
	adts.data = new char[adts.size];
	adts.data[0] = 0xff;
	adts.data[1] = 0xf1;
	adts.data[2] = (((asc.audioObjectType - 1) & 3) << 6) |
		(asc.samplingFrequencyIndex << 2) | (asc.channelConfiguration >> 2);
	adts.data[3] = ((asc.channelConfiguration & 0x3) << 6) | (((frameLength + 7) >> 11) & 0x3);
	adts.data[4] = (((frameLength + 7) >> 3) & 0xff);
	adts.data[5] = (((frameLength + 7) & 0x7) << 5) | 0x1f;
	adts.data[6] = 0xfc;
}
