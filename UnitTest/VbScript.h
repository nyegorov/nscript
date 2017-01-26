#pragma once

#include <ActivScp.h>
#include <comdef.h>
#include <comdefsp.h>
#include <map>

#define IActiveScriptParse	IActiveScriptParse32
_COM_SMARTPTR_TYPEDEF(IActiveScriptParse, __uuidof(IActiveScriptParse));

const IID CLSID_VBScript = {0xb54f3741, 0x5b07, 0x11cf, {0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8 } };
const IID CLSID_PerlScript = {0xF8D77580, 0x0F09, 0x11d0, {0xaa, 0x61, 0x3c, 0x28, 0x4e, 0x0, 0x0, 0x0 } };

class CVbScript : IActiveScriptSite/*, IActiveScriptSiteWindow*/
{
public:
	CVbScript();
	~CVbScript();
	bool Init(const IID scriptId);
	void AddVariable(LPCWSTR szName, const _variant_t& vValue);
	void AddFunction(LPCWSTR szName, LPCWSTR aszArgs[], LPCWSTR szBody);
	void AddObject(LPCWSTR szName, IDispatch *pdisp);
	_variant_t Exec(_bstr_t sScript, bool bRunScript = false);
	void Close();
protected:
	STDMETHOD_(ULONG, AddRef)()							{return 42;}
	STDMETHOD_(ULONG, Release)()						{return 42;}
	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);
	STDMETHOD(GetLCID)(LCID *plcid)						{return E_NOTIMPL;}
	STDMETHOD(GetItemInfo)(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti);
	STDMETHOD(GetDocVersionString)(BSTR *pszVersion)	{return E_NOTIMPL;}
	STDMETHOD(RequestItems)(void)						{return S_OK;}
	STDMETHOD(RequestTypeLibs)(void)					{return S_OK;}
	STDMETHOD(OnScriptTerminate)(const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo)	{return S_OK;}
	STDMETHOD(OnStateChange)(SCRIPTSTATE ssScriptState)	{return S_OK;}
	STDMETHOD(OnScriptError)(IActiveScriptError *pscripterror);
	STDMETHOD(OnEnterScript)(void)						{return S_OK;}
	STDMETHOD(OnLeaveScript)(void)						{return S_OK;}

private:
	IActiveScriptPtr		m_pEngine;
	IActiveScriptParsePtr	m_pParser;
	IID						m_scriptId;
	std::map<std::wstring, IDispatchPtr>	m_mapObjects;
};
