// FileReader.h: interface for the FileReader class.
//
//////////////////////////////////////////////////////////////////////

#ifndef FILEREADER_H
#define FILEREADER_H

#include "StdAfx.h"

class FileReader  
{
public:
	FileReader();
	virtual ~FileReader();

	BOOL Open(LPCWSTR filename);
	void Close();

	BOOL ReadLine(LPWSTR &pStr);

private:
	BOOL ReadMore();

	HANDLE m_hFile;
	LPWSTR m_pBuffer;
	LPWSTR m_pExtBuffer;
};

#endif
