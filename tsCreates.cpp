#include "tsCreates.h"
#include "log4z.h"
#include "mp3Header.h"
#include <iostream>

const int tsCreater::PMT_ID = 0xfff;
const int tsCreater::Video_ID = 0x100;
const int tsCreater::Audio_ID = 0x101;
const int tsCreater::TS_length = 188;
const int tsCreater::PCR_HZ = 27000000;
bool tsCreater::crc32Inited = false;
unsigned int tsCreater::CRC32Table[256];

int MakeTable(uint32_t *crc32_table)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t k = 0;
		for (uint32_t j = (i << 24) | 0x800000; j != 0x80000000; j <<= 1) {
			k = (k << 1) ^ (((k ^ j) & 0x80000000) ? 0x04c11db7 : 0);
		}

		crc32_table[i] = k;
	}
	return 0;
}

uint32_t Crc32Calculate(uint8_t *buffer, uint32_t size, uint32_t *crc32_table)
{
	uint32_t crc32_reg = 0xFFFFFFFF;
	for (uint32_t i = 0; i < size; i++) {
		crc32_reg = (crc32_reg << 8) ^ crc32_table[((crc32_reg >> 24) ^ *buffer++) & 0xFF];
	}
	return crc32_reg;
}

tsCreater::tsCreater(std::string name, bool saveToDisk) :m_tsVcounts(0), m_tsAcounts(0), m_saveToDisk(saveToDisk),
m_name(name), m_started(false), m_firstFrameTime(-1),m_pcrTimes(0), m_audioPts(0), m_fp(nullptr)
{

}

void tsCreater::InitCRC32Table()
{
	MakeTable(CRC32Table);
	crc32Inited = true;
}

void tsCreater::addTimedPacket(const timedPacket & pkt)
{
	if (false == GetAVHeader(pkt))
	{
		return;
	}
	if (-1 == m_firstFrameTime)
	{
		m_firstFrameTime = pkt.timestamp;
	}
	unsigned int frameTime = pkt.timestamp - m_firstFrameTime;
	if (false == m_started)
	{
		AddPatPmt();
		m_started = true;
	}

	if (flv_pkt_audio == pkt.packetType)
	{
		if (nullptr==m_audioHeader.data)
		{
			m_audioHeader.size = pkt.data.front().size;
			m_audioHeader.data = new char[pkt.data.front().size];
			memcpy(m_audioHeader.data, pkt.data.front().data, pkt.data.front().size);
			parseAudioType((unsigned char*)pkt.data.front().data);
		}
		else
		{
			addTSFrame(pkt.data.front(), frameTime, flv_pkt_audio);
		}
	}
	else if (flv_pkt_video == pkt.packetType)
	{
		addTSFrame(pkt.data.front(), frameTime, flv_pkt_video);
	}
}

tsCreater::~tsCreater()
{

	if (m_tsCache.size() > 0)
	{
		if (m_saveToDisk == true)
		{
			m_fp = fopen(m_name.c_str(), "wb");
			if (m_fp)
			{
				for (auto i : m_tsCache)
				{
					fwrite(i, TS_length, 1, m_fp);
					delete[]i;
				}
			}
		}
		m_tsCache.clear();
	}
	if (m_fp != nullptr)
	{
		fclose(m_fp);
		m_fp = nullptr;
	}

	if (m_sps.data)
	{
		delete[]m_sps.data;
		m_sps.data = nullptr;
	}
	if (m_pps.data)
	{
		delete[]m_pps.data;
		m_pps.data = nullptr;
	}
	if (m_sei.data)
	{
		delete[]m_sei.data;
		m_sei.data = nullptr;
	}
}

void tsCreater::addTSFrame(const DataPacket & data, unsigned int timeMS, flvPacketType pktType)
{
	bool addPCR, addDts;

	addDts = flv_pkt_audio == pktType ? false : true;
	if (m_pcrTimes==0)
	{
		addPCR = true;
	}
	else
	{
		addPCR = false;
	}
	if (flv_pkt_video==pktType)
	{
		if (m_pcrTimes++==4)
		{
			m_pcrTimes = 0;
			AddPatPmt();
		}
	}
	
	//addPCR = true;
	int tsCount, padSize;

	unsigned int tmp32;
	unsigned short tmp16;

	//计算真实数据的大小
	DataPacket payload;

	if (flv_pkt_audio == pktType)
	{
		//音频，没有额外数据，第一个字节去掉即可
		/*payload.size = data.size - 1;
		payload.data = new char[payload.size];
		memcpy(payload.data, data.data + 1, payload.size);*/
		if (m_audio_type_id==0x0f)
		{
			//AAC
			DataPacket adts;
			generateADTSHeader(m_asc, data.size - 2, adts);
			payload.size = adts.size + data.size - 2;
			payload.data = new char[payload.size];
			memcpy(payload.data, adts.data, adts.size);
			memcpy(payload.data + adts.size, data.data + 2, data.size - 2);
			delete[]adts.data;
		}
		else if (m_audio_type_id==0x03)
		{
			//MP3 MPEG1
			payload.size = data.size - 1;
			payload.data = new char[payload.size];
			memcpy(payload.data, data.data+1, payload.size);
		}
		else if (m_audio_type_id==0x04)
		{
			//MP3 MPEG2
			payload.size = data.size - 1;
			payload.data = new char[payload.size];
			memcpy(payload.data, data.data + 1, payload.size);
		}
		else
		{
			LOGW("not support audio format " << m_audio_type_id);
		}
		//其他格式未支持

	}
	else if (flv_pkt_video == pktType)
	{
		//视频，第一个字节给类型，然后可能是AVC，可能是4字节大小+数据
		if (data.data[0] == 0x17 && 0 == data.data[1])
		{
			//AVC 提取SPS PPS 
			parseAVC(data);
			return;
		}
		else
		{
			//可能有多个帧，需要提取
			int nalCur = 5;
			while (nalCur < data.size)
			{
				unsigned int nalSize = 0;
				unsigned char *ptrSize = (unsigned char*)(data.data + nalCur);
				unsigned char sizeArray[4];
				sizeArray[0] = ptrSize[0];
				sizeArray[1] = ptrSize[1];
				sizeArray[2] = ptrSize[2];
				sizeArray[3] = ptrSize[3];
				nalSize = (ptrSize[0] << 24) | (ptrSize[1] << 16) |
					(ptrSize[2] << 8) | (ptrSize[3] << 0);
				nalCur += 4;
				int nalType = data.data[nalCur] & 0x1f;
				if (6 == nalType)
				{
					//SEI
					if (m_sei.data != nullptr)
					{
						delete[]m_sei.data;
						m_sei.data = nullptr;
					}
					m_sei.size = nalSize;
					m_sei.data = new char[m_sei.size];
					memcpy(m_sei.data, data.data + nalCur, m_sei.size);
					nalCur += m_sei.size;
				}
				else if (7 == nalType || 8 == nalType)
				{
					//忽略
					nalCur += 4 + nalSize;
				}
				//同一时间只有一个有效帧
				else if (5 == nalType)
				{
					//加sps pps sei
					payload.size = nalSize + 10 + m_sps.size + 4 + m_pps.size + 4;
					if (m_sei.size)
					{
						payload.size += m_sei.size + 4;
					}
					payload.data = new char[payload.size];
					tmp32 = 0;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;
					payload.data[tmp32++] = 0x09;
					payload.data[tmp32++] = 0x10;

					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;
					memcpy(payload.data + tmp32, m_sps.data, m_sps.size);
					tmp32 += m_sps.size;

					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;
					memcpy(payload.data + tmp32, m_pps.data, m_pps.size);
					tmp32 += m_pps.size;

					if (m_sei.size)
					{
						payload.data[tmp32++] = 0x00;
						payload.data[tmp32++] = 0x00;
						payload.data[tmp32++] = 0x00;
						payload.data[tmp32++] = 0x01;
						memcpy(payload.data + tmp32, m_sei.data, m_sei.size);
						tmp32 += m_sei.size;
					}

					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;

					memcpy(payload.data + tmp32, data.data + nalCur, nalSize);
					tmp32 += nalSize;

					break;
				}
				else
				{
					payload.size = nalSize + 10;
					payload.data = new char[payload.size];
					tmp32 = 0;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;
					payload.data[tmp32++] = 0x09;
					payload.data[tmp32++] = 0x10;

					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x00;
					payload.data[tmp32++] = 0x01;
					memcpy(payload.data + tmp32, data.data + nalCur, nalSize);
					tmp32 += nalSize;
					break;
				}
			}
		}
	}

	getTSCount(payload.size, addPCR, addDts, tsCount, padSize);

	unsigned char tsBuf[TS_length];
	int cur = 0;

	if (1 == tsCount)
	{
		memset(tsBuf, 0xff, TS_length);
		cur = 0;
		//四字节头
		tsBuf[cur++] = 0x47;
		if (flv_pkt_audio == pktType)
		{
			tmp16 = 0x4000 | Audio_ID;
		}
		else
		{
			tmp16 = 0x4000 | Video_ID;
		}
		tsBuf[cur++] = tmp16 >> 8;
		tsBuf[cur++] = tmp16 & 0xff;
		if (addPCR || padSize > 0)
		{
			if (flv_pkt_audio == pktType)
			{
				tsBuf[cur++] = 0x30 | m_tsAcounts;
			}
			else
			{
				tsBuf[cur++] = 0x30 | m_tsVcounts;
			}
		}
		else
		{
			if (flv_pkt_audio == pktType)
			{
				tsBuf[cur++] = 0x10 | m_tsAcounts;
			}
			else
			{
				tsBuf[cur++] = 0x10 | m_tsVcounts;
			}
		}
		if (flv_pkt_audio == pktType)
		{
			if (++m_tsAcounts == 16)
			{
				m_tsAcounts = 0;
			}
		}
		else if (flv_pkt_video == pktType)
		{
			if (++m_tsVcounts == 16)
			{
				m_tsVcounts = 0;
			}
		}
		//!四字节头
		//PCR、PAD 
		long long pcr = ((timeMS*(PCR_HZ / 1000)) / 300) % 0x200000000;
		if (addPCR)
		{
			int adpLength = 7 + padSize;//包括pad和adapation field length
			tsBuf[cur++] = adpLength;
			tsBuf[cur++] = 0x10;//PCR flag
			tsBuf[cur++] = ((pcr & 0xfe000000) >> 25);
			tsBuf[cur++] = ((pcr & 0x1fe0000) >> 17);
			tsBuf[cur++] = ((pcr & 0x1fe00) >> 9);
			tsBuf[cur++] = ((pcr & 0x1fe) >> 1);
			tsBuf[cur++] = (((pcr & 1) << 7) | 0x7e);
			tsBuf[cur++] = 0;
			/*tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x7e;
			tsBuf[cur++] = 0x00;*/
			cur += padSize;
		}
		else if (!addPCR&&padSize > 0)
		{
			int adpLength = padSize - 1;
			tsBuf[cur++] = adpLength;
			if (padSize > 1)
			{
				tsBuf[cur] = 0;//flag
				cur += padSize - 1;
			}
		}
		//!PCR、PAD
		//PES
		//如果长度小于65536，长度有效
		tsBuf[cur++] = 0x00;
		tsBuf[cur++] = 0x00;
		tsBuf[cur++] = 0x01;
		if (flv_pkt_audio == pktType)
		{
			tsBuf[cur++] = 0xc0;
			tmp16 = payload.size + 3 + 5;
			tsBuf[cur++] = (tmp16 >> 8) & 0xff;
			tsBuf[cur++] = (tmp16 >> 0) & 0xff;
			tsBuf[cur++] = 0x80;//
			tsBuf[cur++] = 0x80;//pts only
			tsBuf[cur++] = 0x05;//length

			long long audioPtsDelta = 90000 * m_audioFrameSize / m_audioSampleHz;
			m_audioPts += audioPtsDelta;
			tsBuf[cur++] = (0x20) | ((m_audioPts & 0x1c0000000) >> 29) | 1;
			tmp16 = ((m_audioPts & 0x3fff8000) >> 14) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;
			tmp16 = ((m_audioPts & 0x7fff) << 1) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;

			memcpy(tsBuf + cur, payload.data, payload.size);
			cur += payload.size;
		}
		else
		{
			tsBuf[cur++] = 0xe0;
			tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x00;
			tsBuf[cur++] = 0x80;
			tsBuf[cur++] = 0xc0;
			tsBuf[cur++] = 0x0a;

			tsBuf[cur++] = (3 << 4) | ((pcr & 0x1c0000000) >> 29) | 1;
			tmp16 = ((pcr & 0x3fff8000) >> 14) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;
			tmp16 = ((pcr & 0x7fff) << 1) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;
			tsBuf[cur++] = (1 << 4) | ((pcr & 0x1c0000000) >> 29) | 1;
			tmp16 = ((pcr & 0x3fff8000) >> 14) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;
			tmp16 = ((pcr & 0x7fff) << 1) | 1;
			tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
			tsBuf[cur++] = tmp16 & 0xff;

			memcpy(tsBuf + cur, payload.data, payload.size);
			cur += payload.size;
		}

		//!PES
		this->appendTsPkt(tsBuf);
	}
	else
	{
		//不止一个包的情况
		int payloadCur = 0;
		for (int i = 0; i < tsCount; i++)
		{
			memset(tsBuf, 0xff, TS_length);
			cur = 0;
			//第一帧
			if (0 == i)
			{
				//四字节头
				tsBuf[cur++] = 0x47;
				if (flv_pkt_audio == pktType)
				{
					tmp16 = 0x4000 | Audio_ID;
				}
				else
				{
					tmp16 = 0x4000 | Video_ID;
				}
				tsBuf[cur++] = tmp16 >> 8;
				tsBuf[cur++] = tmp16 & 0xff;
				if (addPCR)
				{
					if (flv_pkt_audio == pktType)
					{
						tsBuf[cur++] = 0x30 | m_tsAcounts;
					}
					else
					{
						tsBuf[cur++] = 0x30 | m_tsVcounts;
					}
				}
				else
				{
					if (flv_pkt_audio == pktType)
					{
						tsBuf[cur++] = 0x10 | m_tsAcounts;
					}
					else
					{
						tsBuf[cur++] = 0x10 | m_tsVcounts;
					}
				}

				if (flv_pkt_audio == pktType)
				{
					if (++m_tsAcounts == 16)
					{
						m_tsAcounts = 0;
					}
				}
				else if (flv_pkt_video == pktType)
				{
					if (++m_tsVcounts == 16)
					{
						m_tsVcounts = 0;
					}
				}

				//!四字节头
				//PCR
				long long pcr = ((timeMS*(PCR_HZ / 1000)) / 300) % 0x200000000;
				if (addPCR)
				{
					int adpLength = 7;//包括pad和adapation field length
					tsBuf[cur++] = adpLength;
					tsBuf[cur++] = 0x10;//PCR flag
					tsBuf[cur++] = ((pcr & 0xfe000000) >> 25);
					tsBuf[cur++] = ((pcr & 0x1fe0000) >> 17);
					tsBuf[cur++] = ((pcr & 0x1fe00) >> 9);
					tsBuf[cur++] = ((pcr & 0x1fe) >> 1);
					tsBuf[cur++] = (((pcr & 1) << 7) | 0x7e);
					tsBuf[cur++] = 0;
				}
				//!PCR

				//PES头
				tsBuf[cur++] = 0x00;
				tsBuf[cur++] = 0x00;
				tsBuf[cur++] = 0x01;
				if (flv_pkt_audio == pktType)
				{
					tsBuf[cur++] = 0xc0;
					tmp16 = payload.size + 3 + 5;
					tsBuf[cur++] = (tmp16 >> 8) & 0xff;
					tsBuf[cur++] = (tmp16 >> 0) & 0xff;
					tsBuf[cur++] = 0x80;//
					tsBuf[cur++] = 0x80;//pts only
					tsBuf[cur++] = 0x05;//length

					long long audioPtsDelta = 90000 * m_audioFrameSize / m_audioSampleHz;
					m_audioPts += audioPtsDelta;
					tsBuf[cur++] = (0x20) | ((m_audioPts & 0x1c0000000) >> 29) | 1;
					tmp16 = ((m_audioPts & 0x3fff8000) >> 14) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
					tmp16 = ((m_audioPts & 0x7fff) << 1) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
				}
				else
				{
					tsBuf[cur++] = 0xe0;
					tsBuf[cur++] = 0x00;
					tsBuf[cur++] = 0x00;
					tsBuf[cur++] = 0x80;
					tsBuf[cur++] = 0xc0;
					tsBuf[cur++] = 0x0a;

					tsBuf[cur++] = (3 << 4) | ((pcr & 0x1c0000000) >> 29) | 1;
					tmp16 = ((pcr & 0x3fff8000) >> 14) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
					tmp16 = ((pcr & 0x7fff) << 1) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
					tsBuf[cur++] = (1 << 4) | ((pcr & 0x1c0000000) >> 29) | 1;
					tmp16 = ((pcr & 0x3fff8000) >> 14) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
					tmp16 = ((pcr & 0x7fff) << 1) | 1;
					tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
					tsBuf[cur++] = tmp16 & 0xff;
				}
				//!PES头
				memcpy(tsBuf + cur, payload.data + payloadCur, TS_length - cur);
				payloadCur += TS_length - cur;
				this->appendTsPkt(tsBuf);
			}
			else
			{
				//四字节头
				tsBuf[cur++] = 0x47;
				if (flv_pkt_audio == pktType)
				{
					tmp16 = Audio_ID;
				}
				else
				{
					tmp16 = Video_ID;
				}
				tsBuf[cur++] = tmp16 >> 8;
				tsBuf[cur++] = tmp16 & 0xff;

				//!3字节头
				if (i == tsCount - 1 && padSize != 0)
				{
					//最后一帧，且有pad
					if (flv_pkt_audio == pktType)
					{
						tsBuf[cur++] = 0x30 | m_tsAcounts;
					}
					else
					{
						tsBuf[cur++] = 0x30 | m_tsVcounts;
					}
					tsBuf[cur++] = padSize - 1;
					if (padSize != 1)
					{
						tsBuf[cur++] = 0;
					}
					memcpy(tsBuf + 4 + padSize, payload.data + payloadCur, TS_length - 4 - padSize);
					payloadCur += TS_length - 4 - padSize;
				}
				else
				{
					//普通添加数据
					if (flv_pkt_audio == pktType)
					{
						tsBuf[cur++] = 0x10 | m_tsAcounts;
					}
					else
					{
						tsBuf[cur++] = 0x10 | m_tsVcounts;
					}
					memcpy(tsBuf + cur, payload.data + payloadCur, TS_length - cur);
					payloadCur += TS_length - cur;
				}
				if (flv_pkt_audio == pktType)
				{
					if (++m_tsAcounts == 16)
					{
						m_tsAcounts = 0;
					}
				}
				else if (flv_pkt_video == pktType)
				{
					if (++m_tsVcounts == 16)
					{
						m_tsVcounts = 0;
					}
				}
				appendTsPkt(tsBuf);
			}
		}
	}

	delete[]payload.data;
	payload.data = nullptr;
}

void tsCreater::getTSCount(int dataSize, bool addPCR, bool addDts, int & tsCount, int & padSize)
{
	//第一帧的空间,固定头+PCR+DTS+PTS
	int firstValidSize = TS_length - 4;
	if (addPCR)
	{
		firstValidSize -= 8;
	}
	if (addDts)
	{
		firstValidSize -= 19;
	}
	else
	{
		firstValidSize -= 14;
	}
	//其他帧的空间,只有四个字节的固定头
	int validSize = TS_length - 4;
	//只有一帧的情况
	if (dataSize <= firstValidSize)
	{
		tsCount = 1;
		padSize = firstValidSize - dataSize;
		return;
	}
	dataSize -= firstValidSize;

	tsCount = dataSize / validSize + 1;
	padSize = dataSize%validSize;
	if (padSize != 0)
	{
		tsCount++;
		padSize = validSize - padSize;
	}
	return;
}

bool tsCreater::appendTsPkt(const unsigned char* tsPkt)
{
	char *tmp = new char[TS_length];
	memcpy(tmp, tsPkt, TS_length);
	m_tsCache.push_back(tmp);
	return true;
}

bool tsCreater::GetAVHeader(const timedPacket & pkt)
{
	if (m_audioHeader.data != nullptr&&m_videoHeader.data != nullptr)
	{
		return true;
	}
	auto data = pkt.data.front();
	if (pkt.packetType == flv_pkt_audio)
	{
		if (m_audioHeader.data != nullptr)
		{
			return true;
		}
		m_audioHeader.size = data.size;
		m_audioHeader.data = new char[data.size];
		memcpy(m_audioHeader.data, data.data, data.size);
		parseAudioType((unsigned char*)data.data);
		return false;
	}
	if (pkt.packetType == flv_pkt_video)
	{
		if (m_videoHeader.data != nullptr)
		{
			return true;
		}
		m_videoHeader.size = data.size;
		m_videoHeader.data = new char[data.size];
		memcpy(m_videoHeader.data, data.data, data.size);
		parseAVC(data);
		m_video_type_id = 0x1b;
		return false;
	}
	return false;
}

void tsCreater::parseAudioType(unsigned char *data)
{
	int audioCodec = data[0] >> 4;
	switch (audioCodec)
	{
	case 2://mp3
		m_audioFrameSize = 1152;
		break;
	case 10://aac
		m_audioFrameSize = 1024;
		m_audio_type_id = 0x0f;
		break;
	default:
		break;
	}
	int soundRate = ((data[0] & 0xc) >> 2);
	if (audioCodec == 10)//aac
	{
		InitAudioSpecificConfig(data + 2, m_asc);

		m_audioSampleHz = m_asc.samplingFrequency;
		/*switch (soundRate)
		{
		case 0:
			m_audioSampleHz = 0;
			break;
		case 1:
			m_audioSampleHz = 11025;
			break;
		case 2:
			m_audioSampleHz = 22050;
			break;
		case 3:
			m_audioSampleHz = 44100;
			break;
		default:
			break;
		}*/
	}
	else if (audioCodec == 2)
	{
		auto mp3Header = parseMP3Header(data + 1);
		m_audioSampleHz = mp3Header.SampleRate;
		if (mp3Header.version == 2)
		{
			//MPEG2
			m_audio_type_id = 0x04;
		}
		else if (mp3Header.version == 3)
		{
			//MPEG1
			m_audio_type_id = 0x03;
		}
		else
		{
			LOGF("invalid mp3 mpeg type:" << mp3Header.version);
			m_audio_type_id = 0x4;
		}
	}
	else
	{
		LOGF("unknow audio format :" << audioCodec);
	}
}

void tsCreater::parseAVC(DataPacket  data)
{
	if (data.data[0] == 0x17 && data.data[1] == 0)
	{
		int cur = 10;
		int numOfSequenceParameterSets = data.data[cur++] & 0x1f;
		for (int i = 0; i < numOfSequenceParameterSets; i++)
		{
			if (m_sps.data)
			{
				delete[]m_sps.data;
				m_sps.data = nullptr;
			}
			m_sps.size = (int(data.data[cur]) << 8) | (int(data.data[cur + 1]));
			cur += 2;
			m_sps.data = new char[m_sps.size];
			memcpy(m_sps.data, data.data + cur, m_sps.size);
			cur += m_sps.size;
		}
		int numOfPictureParameterSets = data.data[cur++];
		for (int i = 0; i < numOfPictureParameterSets; i++)
		{
			if (m_pps.data)
			{
				delete[]m_pps.data;
				m_pps.data = nullptr;
			}
			m_pps.size = (int(data.data[cur]) << 8) | (int(data.data[cur + 1]));
			cur += 2;
			m_pps.data = new char[m_pps.size];
			memcpy(m_pps.data, data.data + cur, m_pps.size);
			cur += m_pps.size;
		}
	}
	else
	{
		return;
	}
}

void tsCreater::AddPatPmt() {
	unsigned char tsBuf[TS_length];
	memset(tsBuf, 0xff, TS_length);
	int cur = 0;
	unsigned short tmp16;
	unsigned int tmp32;
	//pat
	tsBuf[cur++] = 0x47;
	tsBuf[cur++] = 0x40;
	tsBuf[cur++] = 0x00;
	tsBuf[cur++] = 0x10;

	tsBuf[cur++] = 0x00;//0个补充字节

	tsBuf[cur++] = 0x00;//table id
	tmp16 = (((0xb0) << 8) | 0xd);//section length
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tsBuf[cur++] = 0x00;//transport stream id
	tsBuf[cur++] = 0x01;
	tsBuf[cur++] = 0xc1;//vesion 0,current valid
	tsBuf[cur++] = 0x00;//section num
	tsBuf[cur++] = 0x00;//last section num
	tsBuf[cur++] = 0x00;//program num
	tsBuf[cur++] = 0x01;
	tmp16 = (0xe000 | PMT_ID);//PMT id
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tmp32 = Crc32Calculate(tsBuf + 5, cur - 5, CRC32Table); //CRC
	tsBuf[cur++] = ((tmp32 >> 24) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 16) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 0) & 0xff);

	if (false == appendTsPkt(tsBuf))
	{
		LOGE("add PAT failed");
		return;
	}
	//pmt
	memset(tsBuf, 0xff, TS_length);
	cur = 0;

	tsBuf[cur++] = 0x47;
	tmp16 = ((0x40 << 8) | PMT_ID);
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tsBuf[cur++] = 0x10;

	tsBuf[cur++] = 0x00;//0个补充字节

	tsBuf[cur++] = 0x02;//table id

	tmp16 = ((0xb0 << 8) | 0x17);//section length
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tsBuf[cur++] = 0x00;//transport stream id
	tsBuf[cur++] = 0x01;
	tsBuf[cur++] = 0xc1;//vesion 0,current valid
	tsBuf[cur++] = 0x00;//section num
	tsBuf[cur++] = 0x00;//last section num
	tmp16 = (0xe000 | Video_ID); //pcr pid
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tmp16 = 0xf000;//program info length = 0
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	//video
	tsBuf[cur++] = m_video_type_id;
	tmp16 = (0xe000 | Video_ID);
	//tmp16 = (0xe000 | 0x1fff);
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tsBuf[cur++] = 0xf0;
	tsBuf[cur++] = 0x00;
	//audio
	tsBuf[cur++] = m_audio_type_id;
	tmp16 = (0xe000 | Audio_ID);
	tsBuf[cur++] = ((tmp16 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp16 >> 0) & 0xff);
	tsBuf[cur++] = 0xf0;
	tsBuf[cur++] = 0x00;

	tmp32 = Crc32Calculate(tsBuf + 5, cur - 5, CRC32Table); //CRC
	tsBuf[cur++] = ((tmp32 >> 24) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 16) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 8) & 0xff);
	tsBuf[cur++] = ((tmp32 >> 0) & 0xff);

	if (false == appendTsPkt(tsBuf))
	{
		LOGE("add PAT failed");
		return;
	}
}