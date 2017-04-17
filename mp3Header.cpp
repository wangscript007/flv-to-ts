#include "mp3Header.h"
#include "log4z.h"

MP3Header parseMP3Header(unsigned char * data)
{
	MP3Header header;
	//sync
	if (data[0] != 0xff || (data[1] & 0xe0) != 0xe0) {
		return header;
	}
	header.version = ((data[1] & 0x18) >> 3);
	header.layer = ((data[1] & 0x6) >> 1);
	header.bitrate = ((data[2] & 0xf0) >> 4);
	header.samplerate = ((data[2] & 0xc) >> 2);
	header.channelMode = ((data[3] & 0xc0) >> 6);
	//计算比特率和采样率和声道
	//bitrate
	switch (header.version) {
		//MPEG2.5
	case 0:
		switch (header.layer) {
			//layer 3
		case 1:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 8;
				break;
			case 2:
				header.Bitrate = 16;
				break;
			case 3:
				header.Bitrate = 24;
				break;
			case 4:
				header.Bitrate = 32;
				break;
			case 5:
				header.Bitrate = 40;
				break;
			case 6:
				header.Bitrate = 48;
				break;
			case 7:
				header.Bitrate = 56;
				break;
			case 8:
				header.Bitrate = 64;
				break;
			case 9:
				header.Bitrate = 80;
				break;
			case 10:
				header.Bitrate = 96;
				break;
			case 11:
				header.Bitrate = 112;
				break;
			case 12:
				header.Bitrate = 128;
				break;
			case 13:
				header.Bitrate = 144;
				break;
			case 14:
				header.Bitrate = 160;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 2
		case 2:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 8;
				break;
			case 2:
				header.Bitrate = 16;
				break;
			case 3:
				header.Bitrate = 24;
				break;
			case 4:
				header.Bitrate = 32;
				break;
			case 5:
				header.Bitrate = 40;
				break;
			case 6:
				header.Bitrate = 48;
				break;
			case 7:
				header.Bitrate = 56;
				break;
			case 8:
				header.Bitrate = 64;
				break;
			case 9:
				header.Bitrate = 80;
				break;
			case 10:
				header.Bitrate = 96;
				break;
			case 11:
				header.Bitrate = 112;
				break;
			case 12:
				header.Bitrate = 128;
				break;
			case 13:
				header.Bitrate = 160;
				break;
			case 14:
				header.Bitrate = 384;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 1
		case 3:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 48;
				break;
			case 3:
				header.Bitrate = 56;
				break;
			case 4:
				header.Bitrate = 64;
				break;
			case 5:
				header.Bitrate = 80;
				break;
			case 6:
				header.Bitrate = 96;
				break;
			case 7:
				header.Bitrate = 112;
				break;
			case 8:
				header.Bitrate = 128;
				break;
			case 9:
				header.Bitrate = 144;
				break;
			case 10:
				header.Bitrate = 160;
				break;
			case 11:
				header.Bitrate = 176;
				break;
			case 12:
				header.Bitrate = 192;
				break;
			case 13:
				header.Bitrate = 224;
				break;
			case 14:
				header.Bitrate = 256;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
		}
		//MPEG2
	case 2:
		switch (header.layer) {
			//layer 3
		case 1:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 8;
				break;
			case 2:
				header.Bitrate = 16;
				break;
			case 3:
				header.Bitrate = 24;
				break;
			case 4:
				header.Bitrate = 32;
				break;
			case 5:
				header.Bitrate = 64;
				break;
			case 6:
				header.Bitrate = 80;
				break;
			case 7:
				header.Bitrate = 56;
				break;
			case 8:
				header.Bitrate = 64;
				break;
			case 9:
				header.Bitrate = 128;
				break;
			case 10:
				header.Bitrate = 160;
				break;
			case 11:
				header.Bitrate = 112;
				break;
			case 12:
				header.Bitrate = 128;
				break;
			case 13:
				header.Bitrate = 256;
				break;
			case 14:
				header.Bitrate = 320;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 2
		case 2:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 48;
				break;
			case 3:
				header.Bitrate = 56;
				break;
			case 4:
				header.Bitrate = 64;
				break;
			case 5:
				header.Bitrate = 80;
				break;
			case 6:
				header.Bitrate = 96;
				break;
			case 7:
				header.Bitrate = 112;
				break;
			case 8:
				header.Bitrate = 128;
				break;
			case 9:
				header.Bitrate = 160;
				break;
			case 10:
				header.Bitrate = 192;
				break;
			case 11:
				header.Bitrate = 224;
				break;
			case 12:
				header.Bitrate = 256;
				break;
			case 13:
				header.Bitrate = 320;
				break;
			case 14:
				header.Bitrate = 384;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 1
		case 3:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 64;
				break;
			case 3:
				header.Bitrate = 96;
				break;
			case 4:
				header.Bitrate = 128;
				break;
			case 5:
				header.Bitrate = 160;
				break;
			case 6:
				header.Bitrate = 192;
				break;
			case 7:
				header.Bitrate = 224;
				break;
			case 8:
				header.Bitrate = 256;
				break;
			case 9:
				header.Bitrate = 288;
				break;
			case 10:
				header.Bitrate = 320;
				break;
			case 11:
				header.Bitrate = 352;
				break;
			case 12:
				header.Bitrate = 384;
				break;
			case 13:
				header.Bitrate = 416;
				break;
			case 14:
				header.Bitrate = 448;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
		}
		//MPEG1
	case 3:
		switch (header.layer) {
			//layer 3
		case 1:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 40;
				break;
			case 3:
				header.Bitrate = 48;
				break;
			case 4:
				header.Bitrate = 56;
				break;
			case 5:
				header.Bitrate = 64;
				break;
			case 6:
				header.Bitrate = 80;
				break;
			case 7:
				header.Bitrate = 96;
				break;
			case 8:
				header.Bitrate = 112;
				break;
			case 9:
				header.Bitrate = 128;
				break;
			case 10:
				header.Bitrate = 160;
				break;
			case 11:
				header.Bitrate = 192;
				break;
			case 12:
				header.Bitrate = 224;
				break;
			case 13:
				header.Bitrate = 256;
				break;
			case 14:
				header.Bitrate = 320;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 2
		case 2:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 48;
				break;
			case 3:
				header.Bitrate = 56;
				break;
			case 4:
				header.Bitrate = 64;
				break;
			case 5:
				header.Bitrate = 80;
				break;
			case 6:
				header.Bitrate = 96;
				break;
			case 7:
				header.Bitrate = 112;
				break;
			case 8:
				header.Bitrate = 128;
				break;
			case 9:
				header.Bitrate = 160;
				break;
			case 10:
				header.Bitrate = 192;
				break;
			case 11:
				header.Bitrate = 224;
				break;
			case 12:
				header.Bitrate = 256;
				break;
			case 13:
				header.Bitrate = 320;
				break;
			case 14:
				header.Bitrate = 384;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
			//layer 1
		case 3:
			switch (header.bitrate) {
			case 0:
				header.Bitrate = 0;
				break;
			case 1:
				header.Bitrate = 32;
				break;
			case 2:
				header.Bitrate = 64;
				break;
			case 3:
				header.Bitrate = 96;
				break;
			case 4:
				header.Bitrate = 128;
				break;
			case 5:
				header.Bitrate = 160;
				break;
			case 6:
				header.Bitrate = 192;
				break;
			case 7:
				header.Bitrate = 224;
				break;
			case 8:
				header.Bitrate = 256;
				break;
			case 9:
				header.Bitrate = 288;
				break;
			case 10:
				header.Bitrate = 320;
				break;
			case 11:
				header.Bitrate = 352;
				break;
			case 12:
				header.Bitrate = 384;
				break;
			case 13:
				header.Bitrate = 416;
				break;
			case 14:
				header.Bitrate = 448;
				break;
			case 15:
				header.Bitrate = 0;
				break;
			}
		}
	}

	//samplerate
	switch (header.version) {
		//MPEG2.5
	case 0:
		switch (header.samplerate) {
		case 0:
			header.SampleRate = 11025;
			break;
		case 1:
			header.SampleRate = 12000;
			break;
		case 2:
			header.SampleRate = 8000;
			break;
		}break;
		//MPEG2
	case 2:
		switch (header.samplerate) {
		case 0:
			header.SampleRate = 22050;
			break;
		case 1:
			header.SampleRate = 24000;
			break;
		case 2:
			header.SampleRate = 16000;
			break;
		}break;
		//MPEG1
	case 3:
		switch (header.samplerate) {
		case 0:
			header.SampleRate = 44100;
			break;
		case 1:
			header.SampleRate = 48000;
			break;
		case 2:
			header.SampleRate = 32000;
			break;
		}break;
	default:
		LOGI("invlaid mp3 version:%d" << header.version);
	}
	//channel
	if (header.channelMode == 3) {
		header.Channel = 1;
	}
	else {
		header.Channel = 2;
	}

	return header;
}
