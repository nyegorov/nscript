#pragma once

#include <assert.h>

namespace reg	{

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
	CRegKey() throw();
	CRegKey( CRegKey& key ) throw();
	explicit CRegKey(HKEY hKey) throw();
	~CRegKey() throw();

	CRegKey& operator=( CRegKey& key ) throw();

// Attributes
public:
	operator HKEY() const throw();
	HKEY m_hKey;

// Operations
public:
	LONG SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw();
	LONG SetBinaryValue(LPCTSTR pszValueName, const void* pValue, ULONG nBytes) throw();
	LONG SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw();
	LONG SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) throw();
	LONG SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType = REG_SZ) throw();
	LONG SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw();

	LONG QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw();
	LONG QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) throw();
	LONG QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw();
	LONG QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) throw();
	LONG QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();
	LONG QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw();

	// Get the key's security attributes.
	LONG GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes) throw();
	// Set the key's security attributes.
	LONG SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) throw();

	LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL) throw();
	static LONG WINAPI SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);

	// Create a new registry key (or open an existing one).
	LONG Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
		LPTSTR lpszClass = REG_NONE, DWORD dwOptions = REG_OPTION_NON_VOLATILE,
		REGSAM samDesired = KEY_READ | KEY_WRITE,
		LPSECURITY_ATTRIBUTES lpSecAttr = NULL,
		LPDWORD lpdwDisposition = NULL) throw();
	// Open an existing registry key.
	LONG Open(HKEY hKeyParent, LPCTSTR lpszKeyName,
		REGSAM samDesired = KEY_READ | KEY_WRITE) throw();
	// Close the registry key.
	LONG Close() throw();
	// Flush the key's data to disk.
	LONG Flush() throw();

	// Detach the CRegKey object from its HKEY.  Releases ownership.
	HKEY Detach() throw();
	// Attach the CRegKey object to an existing HKEY.  Takes ownership.
	void Attach(HKEY hKey) throw();

	// Enumerate the subkeys of the key.
	LONG EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime = NULL) throw();
	LONG NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync = TRUE) throw();

	LONG DeleteSubKey(LPCTSTR lpszSubKey) throw();
	LONG RecurseDeleteKey(LPCTSTR lpszKey) throw();
	LONG DeleteValue(LPCTSTR lpszValue) throw();
};

inline CRegKey::CRegKey() throw() : 
	m_hKey( NULL )
{
}

inline CRegKey::CRegKey( CRegKey& key ) throw() :
	m_hKey( NULL )
{
	Attach( key.Detach() );
}

inline CRegKey::CRegKey(HKEY hKey) throw() :
	m_hKey(hKey)
{
}

inline CRegKey::~CRegKey() throw()
{Close();}

inline CRegKey& CRegKey::operator=( CRegKey& key ) throw()
{
    if(m_hKey!=key.m_hKey)
    {
	    Close();
	    Attach( key.Detach() );
    }
	return( *this );
}

inline CRegKey::operator HKEY() const throw()
{return m_hKey;}

inline HKEY CRegKey::Detach() throw()
{
	HKEY hKey = m_hKey;
	m_hKey = NULL;
	return hKey;
}

inline void CRegKey::Attach(HKEY hKey) throw()
{
	assert(m_hKey == NULL);
	m_hKey = hKey;
}

inline LONG CRegKey::DeleteSubKey(LPCTSTR lpszSubKey) throw()
{
	assert(m_hKey != NULL);
	return RegDeleteKey(m_hKey, lpszSubKey);
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue) throw()
{
	assert(m_hKey != NULL);
	return RegDeleteValue(m_hKey, (LPTSTR)lpszValue);
}

inline LONG CRegKey::Close() throw()
{
	LONG lRes = ERROR_SUCCESS;
	if (m_hKey != NULL)
	{
		lRes = RegCloseKey(m_hKey);
		m_hKey = NULL;
	}
	return lRes;
}

inline LONG CRegKey::Flush() throw()
{
	assert(m_hKey != NULL);

	return ::RegFlushKey(m_hKey);
}

inline LONG CRegKey::EnumKey(DWORD iIndex, LPTSTR pszName, LPDWORD pnNameLength, FILETIME* pftLastWriteTime) throw()
{
	FILETIME ftLastWriteTime;

	assert(m_hKey != NULL);
	if (pftLastWriteTime == NULL)
	{
		pftLastWriteTime = &ftLastWriteTime;
	}

	return ::RegEnumKeyEx(m_hKey, iIndex, pszName, pnNameLength, NULL, NULL, NULL, pftLastWriteTime);
}

inline LONG CRegKey::NotifyChangeKeyValue(BOOL bWatchSubtree, DWORD dwNotifyFilter, HANDLE hEvent, BOOL bAsync) throw()
{
	assert(m_hKey != NULL);
	assert((hEvent != NULL) || !bAsync);

	return ::RegNotifyChangeKeyValue(m_hKey, bWatchSubtree, dwNotifyFilter, hEvent, bAsync);
}

inline LONG CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition) throw()
{
	assert(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey = NULL;
	LONG lRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired) throw()
{
	assert(hKeyParent != NULL);
	HKEY hKey = NULL;
	LONG lRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		lRes = Close();
		assert(lRes == ERROR_SUCCESS);
		m_hKey = hKey;
	}
	return lRes;
}

inline LONG CRegKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) throw()
{
	assert(m_hKey != NULL);

	return( ::RegQueryValueEx(m_hKey, pszValueName, NULL, pdwType, static_cast< LPBYTE >( pData ), pnBytes) );
}

inline LONG CRegKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) throw()
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	assert(m_hKey != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}
inline LONG CRegKey::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) throw()
{
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	assert(m_hKey != NULL);

	nBytes = sizeof(ULONGLONG);
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&qwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_QWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) throw()
{
	LONG lRes;
	DWORD dwType;

	assert(pnBytes != NULL);
	assert(m_hKey != NULL);

	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pValue),
		pnBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_BINARY)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw()
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	assert(m_hKey != NULL);
	assert(pnChars != NULL);

	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	
	if (lRes != ERROR_SUCCESS)
	{
		return lRes;
	}

	if(dwType != REG_SZ && dwType != REG_EXPAND_SZ)
	{
		return ERROR_INVALID_DATA;
	}

	if (pszValue != NULL)
	{
		if(nBytes!=0)
		{
			if ((nBytes % sizeof(TCHAR) != 0) || (pszValue[nBytes / sizeof(TCHAR) -1] != 0))
			{
 				return ERROR_INVALID_DATA;
            }
        }
        else
        {
            pszValue[0]=TEXT('\0');
        }
    }

 	*pnChars = nBytes/sizeof(TCHAR);
	
	return ERROR_SUCCESS;
}

inline LONG CRegKey::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) throw()
{
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	assert(m_hKey != NULL);
	assert(pnChars != NULL);

	if (pszValue != NULL && *pnChars < 2)
		return ERROR_INSUFFICIENT_BUFFER;
		
	nBytes = (*pnChars)*sizeof(TCHAR);
	*pnChars = 0;
	
	lRes = ::RegQueryValueEx(m_hKey, pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_MULTI_SZ)
		return ERROR_INVALID_DATA;
	if (pszValue != NULL && (nBytes % sizeof(TCHAR) != 0 || nBytes / sizeof(TCHAR) < 1 || pszValue[nBytes / sizeof(TCHAR) -1] != 0 || ((nBytes/sizeof(TCHAR))>1 && pszValue[nBytes / sizeof(TCHAR) - 2] != 0)))
		return ERROR_INVALID_DATA;

	*pnChars = nBytes/sizeof(TCHAR);

	return ERROR_SUCCESS;
}

inline LONG WINAPI CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	assert(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(hKeyParent, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

inline LONG CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName) throw()
{
	assert(lpszValue != NULL);
	CRegKey key;
	LONG lRes = key.Create(m_hKey, lpszKeyName);
	if (lRes == ERROR_SUCCESS)
		lRes = key.SetStringValue(lpszValueName, lpszValue);
	return lRes;
}

inline LONG CRegKey::SetValue(LPCTSTR pszValueName, DWORD dwType, const void* pValue, ULONG nBytes) throw()
{
	assert(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

inline LONG CRegKey::SetBinaryValue(LPCTSTR pszValueName, const void* pData, ULONG nBytes) throw()
{
	assert(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_BINARY, reinterpret_cast<const BYTE*>(pData), nBytes);
}

inline LONG CRegKey::SetDWORDValue(LPCTSTR pszValueName, DWORD dwValue) throw()
{
	assert(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

inline LONG CRegKey::SetQWORDValue(LPCTSTR pszValueName, ULONGLONG qwValue) throw()
{
	assert(m_hKey != NULL);
	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

inline LONG CRegKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType) throw()
{
	assert(m_hKey != NULL);
	assert(pszValue != NULL);
	assert((dwType == REG_SZ) || (dwType == REG_EXPAND_SZ));

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, dwType, reinterpret_cast<const BYTE*>(pszValue), (lstrlen(pszValue)+1)*sizeof(TCHAR));
}

inline LONG CRegKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) throw()
{
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	assert(m_hKey != NULL);
	assert(pszValue != NULL);

	// Find the total length (in bytes) of all of the strings, including the
	// terminating '\0' of each string, and the second '\0' that terminates
	// the list.
	nBytes = 0;
	pszTemp = pszValue;
	do
	{
		nLength = lstrlen(pszTemp)+1;
		pszTemp += nLength;
		nBytes += nLength*sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx(m_hKey, pszValueName, NULL, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue),
		nBytes);
}

inline LONG CRegKey::GetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, LPDWORD pnBytes) throw()
{
	assert(m_hKey != NULL);
	assert(pnBytes != NULL);

	return ::RegGetKeySecurity(m_hKey, si, psd, pnBytes);
}

inline LONG CRegKey::SetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) throw()
{
	assert(m_hKey != NULL);
	assert(psd != NULL);

	return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline LONG CRegKey::RecurseDeleteKey(LPCTSTR lpszKey) throw()
{
	CRegKey key;
	LONG lRes = key.Open(m_hKey, lpszKey, KEY_READ | KEY_WRITE);
	if (lRes != ERROR_SUCCESS)
	{
		if (lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)	return lRes;
	}
	FILETIME time;
	DWORD dwSize = 256;
	TCHAR szBuffer[256];
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		lRes = key.RecurseDeleteKey(szBuffer);
		if (lRes != ERROR_SUCCESS)
			return lRes;
		dwSize = 256;
	}
	key.Close();
	return DeleteSubKey(lpszKey);
}

}