// FileWriter.h: interface for the FileWriter class.
//
//////////////////////////////////////////////////////////////////////

#ifndef FILEWRITER_H
#define FILEWRITER_H

#include "StdAfx.h"

class FileWriter  
{
public:
	FileWriter();
	virtual ~FileWriter();

	BOOL Open(LPCWSTR filename);
	void Close();

	FileWriter& operator<< (const int& val);
	FileWriter& operator<< (const double& val);
	FileWriter& operator<< (const __int64& val);

	FileWriter& operator<< (const char& val);
	FileWriter& operator<< (const wchar_t& val);

	FileWriter& operator<< (const LPSTR& val);
	FileWriter& operator<< (const LPWSTR& val);

	FileWriter& operator<< (const LPCSTR& val);
	FileWriter& operator<< (const LPCWSTR& val);

	struct FileWriterEOL
	{
	} EOL;

	FileWriter& operator<< (const FileWriterEOL& val);

private:
	HANDLE m_hFile;

};

#endif
