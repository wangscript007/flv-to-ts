#pragma once

#include "util.h"

#include <string>
#include <map>
#include <mutex>
#include <list>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif // WIN32


/*  r ��ֻ���ļ������ļ�������ڡ�
r + �򿪿ɶ�д���ļ������ļ�������ڡ�
w ��ֻд�ļ������ļ��������ļ�������Ϊ0�������ļ����ݻ���ʧ�����ļ��������������ļ���
w + �򿪿ɶ�д�ļ������ļ��������ļ�������Ϊ�㣬�����ļ����ݻ���ʧ�����ļ��������������ļ���
a �Ը��ӵķ�ʽ��ֻд�ļ������ļ������ڣ���Ὠ�����ļ�������ļ����ڣ�д������ݻᱻ�ӵ��ļ�β�����ļ�ԭ�ȵ����ݻᱻ������
a + �Ը��ӷ�ʽ�򿪿ɶ�д���ļ������ļ������ڣ���Ὠ�����ļ�������ļ����ڣ�д������ݻᱻ�ӵ��ļ�β�󣬼��ļ�ԭ�ȵ����ݻᱻ������
*/
//�������Ѿ������ڶ���Ȼ��д�������ֻ����ֻ����дʱ�������
//ʹ��w+��wb+��ʽд�ļ�

//���ļ�ʱʹ�û�����ƣ�һ�ζ���������ݣ�����Ƶ�����ļ�

#define USE_CACHE_READ_MODE 1
#define MIN_FILE_CACHE_SIZE 1024*1024

enum fileOpenMode
{
	open_read,
	open_readBinary,
	open_write,
	open_writeBinary
};

class fileHander;

struct fileMutex
{
	fileMutex();
	ReadWriteLock fileLock;
	int m_ref;
};
class fileHander
{
public:
	fileHander(std::string fileName, fileOpenMode openMode);
	bool openSuccessed();
	//��ʹ�����Լ�����ռ䡣��ȡ���ݣ����ض�ȡ��������
	int readFile(int size, char *destBuf);
	//д�����ݣ�����д���������
	int writeFile(int size, char *srcBuf);
	//��ǰλ��
	long long curFile();
	//seek �ɹ�����0
	int seek(long long offset, int origin);
	long long fileSize();
	~fileHander();

private:
	std::string getOpenMode(fileOpenMode openMode);
	fileHander(const fileHander&) = delete;
	fileHander& operator=(const fileHander&) = delete;
	FILE	*m_fp;
	std::string m_fileName;
	fileOpenMode m_openMode;
	bool	m_openSuccessed;
	bool	m_hasWriter;
	//�ļ�λ��
	long long	m_curFile;
	static ReadWriteLock	s_rwMapLock;
	static std::map<std::string, fileMutex*>	s_rwMap;
#ifdef USE_CACHE_READ_MODE
	//seek to ����λ�ã����¶�ȡbuffer,�Ѿ�����
	int seekToPos(long pos);
	long long	m_fileSize;
	//����������ļ���λ��
	long long	m_cacheStartPos;
	//����ڻ����λ��
	long long	m_readStartPos;
	//���ܲ����������ļ��еĻ����С����Ϊ�����йؼ�֡��������ļ������С
	int		m_readCacheSize;
	char	*m_cacheData;
#endif // USE_CACHE_READ_MODE

};