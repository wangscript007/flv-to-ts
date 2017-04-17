#pragma once

struct MP3Header {
	unsigned char version;
	unsigned char 	layer;
	unsigned char 	bitrate;
	unsigned char 	samplerate;
	unsigned char 	channelMode;

	int	Bitrate;
	int	SampleRate;
	int	Channel;
};

MP3Header parseMP3Header(unsigned char* data);
