// FileWriter.cpp: implementation of the FileWriter class.
//
//////////////////////////////////////////////////////////////////////

#include "FileWriter.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FileWriter::FileWriter()
{
	m_hFile = INVALID_HANDLE_VALUE;
}

FileWriter::~FileWriter()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
		Close();
}

BOOL FileWriter::Open(LPCWSTR filename)
{
	USES_CONVERSION;

	LPWSTR pCurr = (LPWSTR)filename;

	switch (pCurr[0])
	{
	case 0:
		return FALSE;
	case '\\':
		switch (pCurr[1])
		{
		case '\\':
			pCurr += 2;
			break;
		default:
			return FALSE;
		}
		break;
	default:
		switch (pCurr[1])
		{
		case ':':
			switch (pCurr[2])
			{
			case 0:
				return FALSE;
			case '\\':
				pCurr += 3;
				break;
			default:
				return FALSE;
			}
			break;
		default:
			return FALSE;
		}
	}

	LPWSTR pBackslash;
	while (pBackslash = wcschr(pCurr, '\\'))
	{
		LPWSTR pPartial = NULL;
		strCopy(pPartial, filename, pBackslash - filename);

		HANDLE hDir;
		hDir = CreateFile(W2T(pPartial), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hDir == INVALID_HANDLE_VALUE)
		{
			if (!CreateDirectory(W2T(pPartial), NULL))
			{
				delete[] pPartial;
				return FALSE;
			}
			hDir = CreateFile(W2T(pPartial), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if (hDir == INVALID_HANDLE_VALUE)
			{
				delete[] pPartial;
				return FALSE;
			}
		}
		CloseHandle(hDir);
		delete[] pPartial;


		pCurr = pBackslash + 1;
	}

	m_hFile = CreateFile(W2T(filename), GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return (m_hFile != INVALID_HANDLE_VALUE);
}

void FileWriter::Close()
{
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
}


FileWriter& FileWriter::operator<< (const int& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	char str[25];
	sprintf((char*)&str, "%i", val);
	DWORD written = 0;
	WriteFile(m_hFile, str, strlen(str), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const double& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	char str[25];
	sprintf((char*)str, "%f", val);
	DWORD written = 0;
	WriteFile(m_hFile, str, strlen(str), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const __int64& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	char str[25];
	sprintf((char*)str, "%i", val);
	DWORD written = 0;
	WriteFile(m_hFile, str, strlen(str), &written, NULL);
	return *this;
}

//Characters
FileWriter& FileWriter::operator<< (const char& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, &val, 1, &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const wchar_t& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, &val, 1, &written, NULL);
	return *this;
}

//Strings
FileWriter& FileWriter::operator<< (const LPSTR& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, val, strlen(val), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const LPWSTR& val)
{
	USES_CONVERSION;

	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, W2A(val), wcslen(val), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const LPCSTR& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, val, strlen(val), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const LPCWSTR& val)
{
	USES_CONVERSION;

	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, W2A(val), wcslen(val), &written, NULL);
	return *this;
}
FileWriter& FileWriter::operator<< (const FileWriterEOL& val)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return *this;

	DWORD written = 0;
	WriteFile(m_hFile, "\r\n", 2, &written, NULL);
	return *this;
}

