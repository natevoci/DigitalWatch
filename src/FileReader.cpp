// FileReader.cpp: implementation of the FileReader class.
//
//////////////////////////////////////////////////////////////////////

#include "FileReader.h"
#include "GlobalFunctions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FileReader::FileReader()
{
	m_hFile = INVALID_HANDLE_VALUE;
	m_pBuffer = NULL;
	m_pExtBuffer = NULL;
}

FileReader::~FileReader()
{
	if (m_pBuffer)
		delete[] m_pBuffer;

	if (m_pExtBuffer)
		delete[] m_pExtBuffer;

	if (m_hFile != INVALID_HANDLE_VALUE)
		Close();
}

BOOL FileReader::Open(LPCWSTR filename)
{
	USES_CONVERSION;

	m_hFile = CreateFile(W2T(filename), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	return (m_hFile != INVALID_HANDLE_VALUE);
}

void FileReader::Close()
{
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
}

BOOL FileReader::ReadLine(LPWSTR &pStr)
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!m_pBuffer)
	{
		if (!ReadMore())
			return FALSE;
	}

	LPWSTR strEOL = wcspbrk(m_pBuffer, L"\r\n");

	while (strEOL == NULL)
	{
		if (!ReadMore())
			return FALSE;
		strEOL = wcspbrk(m_pBuffer, L"\r\n");
	}

	strCopy(m_pExtBuffer, m_pBuffer, strEOL - m_pBuffer);
	pStr = m_pExtBuffer;

	if ((strEOL[0] == '\r') && (strEOL[1] == '\n'))
		strEOL += 2;
	else
		strEOL++;

	LPWSTR newBuffer = NULL;
	strCopy(newBuffer, strEOL);
	delete[] m_pBuffer;
	m_pBuffer = newBuffer;

	return TRUE;
}

BOOL FileReader::ReadMore()
{
	const int BYTES_TO_READ = 256;

	char tmpBuffer[BYTES_TO_READ + 1];
	ZeroMemory(tmpBuffer, BYTES_TO_READ + 1);

	DWORD bytesRead = 0;
	if (ReadFile(m_hFile, (void*)&tmpBuffer, BYTES_TO_READ, &bytesRead, NULL) == FALSE)
		return FALSE;

	if (bytesRead <= 0)
		return FALSE;

	if (m_pBuffer)
	{
		int length = wcslen(m_pBuffer) + strlen((LPSTR)&tmpBuffer);

		LPWSTR pBuffer = new wchar_t[length + 1];
		swprintf(pBuffer, L"%s%S", m_pBuffer, (LPSTR)&tmpBuffer);

		delete[] m_pBuffer;
		m_pBuffer = pBuffer;
	}
	else
	{
		int length = strlen((LPSTR)&tmpBuffer);
		m_pBuffer = new wchar_t[length + 1];
		mbstowcs(m_pBuffer, (LPSTR)&tmpBuffer, length);
		m_pBuffer[length] = 0;
	}
	return TRUE;
}