#include <iostream>
#include "flvParser.h"
#include "tsCreates.h"
#include "aac.h"

int main(char argc, char **argv) {
	std::string srcName = "aac.flv";
	tsCreater tser("aac.ts");
	tser.InitCRC32Table();
	flvParser reader(srcName);
	AudioSC asc;
	bool firstAudio = true;
	FILE *fpaac = fopen("mp3.mp3", "wb");
	while (true)
	{
		auto pkt = reader.getNextFrame();
		if (pkt.data.size()==0&&pkt.packetType!=flv_pkt_audio)
		{
			break;
		}
		if (pkt.packetType==flv_pkt_video||
			flv_pkt_audio==pkt.packetType)
		{
			tser.addTimedPacket(pkt);
		}
		if (flv_pkt_audio==pkt.packetType)
		{
			/*if (firstAudio)
			{
				InitAudioSpecificConfig((unsigned char*)pkt.data.front().data + 2, asc);
				firstAudio = false;
			}
			else
			{
				DataPacket adts;
				generateADTSHeader(asc, pkt.data.front().size - 2, adts);
				fwrite(adts.data, adts.size, 1, fpaac);
				fwrite(pkt.data.front().data + 2, pkt.data.front().size - 2, 1, fpaac);
			}*/
			if (firstAudio)
			{
				firstAudio = false;
			}
			else
			{
				fwrite(pkt.data.front().data + 1, pkt.data.front().size - 1, 1, fpaac);
			}
		}
		for (auto i:pkt.data)
		{
			if (i.data!=nullptr)
			{
				delete[]i.data;
				i.data = nullptr;
			}
		}
	}
	fclose(fpaac);
	return 1;
}