#pragma once
#include <string>
#include <list>
#include "RTP.h"
#include "aac.h"
//先就写一个单独的ts包
class tsCreater
{
public:
	tsCreater(std::string name, bool saveToDisk = true);
	static void InitCRC32Table();
	void addTimedPacket(const timedPacket &pkt);
	~tsCreater();
private:
	//填充timedPacket
	void addTSFrame(const DataPacket &data, unsigned int timeMS, flvPacketType pktType);
	void getTSCount(int dataSize, bool addPCR,bool addDts, int& tsCount, int &padSize);
	//添加ts帧到缓存中
	bool appendTsPkt(const unsigned char* tsPkt);
	//成功表示可以从这帧开始写数据，失败表示这帧已被保存为音视频头
	bool GetAVHeader(const timedPacket &pkt);
	void parseAudioType(unsigned char *data);
	void parseAVC(DataPacket data);
	//已有音视频头，添加pat pmt
	void AddPatPmt();
private:
	std::string m_name;
	bool m_saveToDisk;
	static const int PMT_ID;
	static const int Video_ID;
	static const int Audio_ID;
	static const int TS_length;
	static const int PCR_HZ;
	static unsigned int CRC32Table[256];
	static bool crc32Inited;
	short m_pcrTimes;
	short m_tsVcounts;//视频ts包计数器
	short m_tsAcounts;//音频ts包计数器
	bool m_started;
	DataPacket m_audioHeader;
	DataPacket m_videoHeader;
	DataPacket m_sps;
	DataPacket m_pps;
	DataPacket m_sei;
	int m_video_type_id;
	int m_audio_type_id;
	int m_audioFrameSize;
	int m_audioSampleHz;
	long long m_audioPts;
	unsigned int m_firstFrameTime;//以第一个时间为起点时间
	std::list<char*>	m_tsCache;
	AudioSC m_asc;
	FILE *m_fp;

};

