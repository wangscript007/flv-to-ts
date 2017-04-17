#pragma once
#include <string>
#include <list>
#include "RTP.h"
#include "aac.h"
//�Ⱦ�дһ��������ts��
class tsCreater
{
public:
	tsCreater(std::string name, bool saveToDisk = true);
	static void InitCRC32Table();
	void addTimedPacket(const timedPacket &pkt);
	~tsCreater();
private:
	//���timedPacket
	void addTSFrame(const DataPacket &data, unsigned int timeMS, flvPacketType pktType);
	void getTSCount(int dataSize, bool addPCR,bool addDts, int& tsCount, int &padSize);
	//���ts֡��������
	bool appendTsPkt(const unsigned char* tsPkt);
	//�ɹ���ʾ���Դ���֡��ʼд���ݣ�ʧ�ܱ�ʾ��֡�ѱ�����Ϊ����Ƶͷ
	bool GetAVHeader(const timedPacket &pkt);
	void parseAudioType(unsigned char *data);
	void parseAVC(DataPacket data);
	//��������Ƶͷ�����pat pmt
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
	short m_tsVcounts;//��Ƶts��������
	short m_tsAcounts;//��Ƶts��������
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
	unsigned int m_firstFrameTime;//�Ե�һ��ʱ��Ϊ���ʱ��
	std::list<char*>	m_tsCache;
	AudioSC m_asc;
	FILE *m_fp;

};

