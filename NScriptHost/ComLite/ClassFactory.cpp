#include "stdafx.h"
#include "ClassFactory.h"
#include "RegKey.h"

namespace com	{

static ClassFactory*g_pFactory  = NULL;
 HINSTANCE	g_instance	= NULL;
static long			g_Locks = 0;
static TCHAR*		g_modelNames[] = {TEXT("None"), TEXT("Apartment"), TEXT("Free"), TEXT("Both"), TEXT("Neutral"), NULL};

#pragma comment(linker, "/EXPORT:DllMain=_DllMain@12,PRIVATE")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow=_DllCanUnloadNow@0,PRIVATE")
#pragma comment(linker, "/EXPORT:DllRegisterServer=_DllRegisterServer@0,PRIVATE")
#pragma comment(linker, "/EXPORT:DllUnregisterServer=_DllUnregisterServer@0,PRIVATE")
#pragma comment(linker, "/EXPORT:DllGetClassObject=_DllGetClassObject@12,PRIVATE")

LONG LockModule(BOOL lock)	{return lock ? InterlockedIncrement(&g_Locks) : InterlockedDecrement(&g_Locks);}
HINSTANCE GetInstance()		{return g_instance;}

extern "C" {
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if(dwReason == DLL_PROCESS_ATTACH)	g_instance = hInstance;
	return TRUE;
}
STDAPI DllCanUnloadNow(void)		{return g_Locks == 0 ? S_OK : S_FALSE;}
STDAPI DllRegisterServer(void)		{return g_pFactory ? g_pFactory->Register()   : E_FAIL;}
STDAPI DllUnregisterServer(void)	{return g_pFactory ? g_pFactory->Unregister() : E_FAIL;}
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)	
{
	return g_pFactory ? g_pFactory->GetFactory(rclsid, riid, ppv) : E_NOINTERFACE;
}
}

ClassFactory::ClassFactory(REFCLSID clsId, LPCTSTR description, ThreadingModel threadingModel, LPCTSTR progId) :
_clsId(clsId), _threadingModel(threadingModel), _description(description), _progId(progId), _next(g_pFactory)	{
	g_pFactory = this;
}

STDMETHODIMP ClassFactory::Register()
{
	LPOLESTR cls_name;
	HRESULT hr;
	TCHAR module[MAX_PATH+1], clsIdString[256];

	if(!GetModuleFileName(g_instance, module+1, MAX_PATH))							return E_FAIL;

	if(FAILED(hr = StringFromCLSID(_clsId, &cls_name)))								return hr;
#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, 0, cls_name, -1, clsIdString, _countof(clsIdString), NULL, NULL);
#else
	lstrcpynW(clsIdString, cls_name, _countof(clsIdString));
#endif
	CoTaskMemFree(cls_name);

	reg::CRegKey key;
	bool restricted = false;

	if( ERROR_SUCCESS != key.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Classes\\CLSID"), KEY_WRITE) || 
		ERROR_SUCCESS != key.Create(key, clsIdString, REG_NONE, 0, KEY_WRITE))		{
		if(ERROR_SUCCESS != key.Open(HKEY_CURRENT_USER, TEXT("Software\\Classes\\CLSID"), KEY_WRITE))		return E_ACCESSDENIED;
		if(ERROR_SUCCESS != key.Create(key, clsIdString, REG_NONE, 0, KEY_WRITE))	return E_ACCESSDENIED;
		restricted = true;
	}
	if(ERROR_SUCCESS != key.SetStringValue(NULL, _description))						return E_ACCESSDENIED;
	if(_progId)	{
		reg::CRegKey keyProgId;
		if(ERROR_SUCCESS != keyProgId.Open(restricted ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE, TEXT("Software\\Classes"), KEY_WRITE))	return E_ACCESSDENIED;
		if(ERROR_SUCCESS != keyProgId.Create(keyProgId, _progId, REG_NONE, 0, KEY_SET_VALUE))			return E_ACCESSDENIED;
		if(ERROR_SUCCESS != keyProgId.SetStringValue(NULL, _description))			return E_ACCESSDENIED;
		if(ERROR_SUCCESS != keyProgId.SetKeyValue(TEXT("CLSID"), clsIdString))		return E_ACCESSDENIED;
		if(ERROR_SUCCESS != key.SetKeyValue(TEXT("ProgID"), _progId))				return E_ACCESSDENIED;
	}
	if((g_instance == NULL) || (g_instance == GetModuleHandle(NULL)))		{
		// out-of-process (EXE)
		module[0] = TEXT('\"');
		module[lstrlen(module) + 1] = 0;
		module[lstrlen(module)] = TEXT('\"');
		if(ERROR_SUCCESS != key.SetKeyValue(TEXT("LocalServer32"), module))			return E_ACCESSDENIED;
	}
	else	{
		// in-process (DLL)
		if(ERROR_SUCCESS != key.SetKeyValue(TEXT("InprocServer32"), module + 1))	return E_ACCESSDENIED;
		if (_threadingModel != tmUnknown)	{
			if(ERROR_SUCCESS != key.SetKeyValue(TEXT("InprocServer32"), g_modelNames[_threadingModel], TEXT("ThreadingModel")))	return E_ACCESSDENIED;
		}
	}

	return _next ? _next->Register() : S_OK;
}

STDMETHODIMP ClassFactory::Unregister()
{
	LPOLESTR cls_name;
	reg::CRegKey key;
	//key.Attach(HKEY_CLASSES_ROOT);

	HKEY hkey = HKEY_CURRENT_USER;
	for(int i=0; i<2; i++)	{
		key.Open(hkey,  TEXT("Software\\Classes"), KEY_READ | KEY_WRITE);
		if(_progId)	key.RecurseDeleteKey(_progId);
		if(SUCCEEDED(StringFromCLSID(_clsId, &cls_name)))	{
			key.Open(key, TEXT("CLSID"), KEY_READ | KEY_WRITE);
#ifndef UNICODE
			TCHAR clsIdString[256];
			WideCharToMultiByte(CP_ACP, 0, cls_name, -1, clsIdString, sizeof(clsIdString), NULL, NULL);
			key.RecurseDeleteKey(clsIdString);
#else
			key.RecurseDeleteKey(cls_name);
#endif
			CoTaskMemFree(cls_name);
		}
		hkey = HKEY_LOCAL_MACHINE;
	}

	if(_next)	_next->Unregister();
	return S_OK;
}

#if _WIN32_WINNT >= 0x0600
__if_not_exists(RegisterTypeLibForUser)	{
WINOLEAUTAPI RegisterTypeLibForUser(ITypeLib * ptlib, OLECHAR  *szFullPath, OLECHAR  *szHelpDir);
WINOLEAUTAPI UnRegisterTypeLibForUser(REFGUID libID, WORD wVerMajor, WORD wVerMinor, LCID lcid, SYSKIND syskind);
}
#endif

STDMETHODIMP TypeLibrary::Register()
{
	HRESULT hr;
	OLECHAR module[MAX_PATH];
	if(!GetModuleFileNameW(g_instance, module, MAX_PATH))				return E_FAIL;
	ITypeLibPtr typelib;
	if(FAILED(hr = LoadTypeLib(module, &typelib)))						return hr;
	if( FAILED(hr = RegisterTypeLib(typelib, module, NULL)))
#if _WIN32_WINNT >= 0x0600
		if(FAILED(hr = RegisterTypeLibForUser(typelib, module, NULL)))		
#endif
		return hr;

	return _next ? _next->Register() : S_OK;
}

STDMETHODIMP TypeLibrary::Unregister()
{
	OLECHAR module[MAX_PATH];
	ITypeLibPtr typelib;
	if(GetModuleFileNameW(g_instance, module, MAX_PATH) && SUCCEEDED(LoadTypeLib(module, &typelib)))	{
		TLIBATTR *ptla;
		typelib->GetLibAttr(&ptla);
#if _WIN32_WINNT >= 0x0600
		UnRegisterTypeLibForUser(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
#endif
		UnRegisterTypeLib(ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind);
	}

	if(_next)	_next->Unregister();
	return S_OK;
}

}