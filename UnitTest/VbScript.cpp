#include "pch.h"
#include <sstream>
#include <ActivScp.h>
#include <comdef.h>
#include <comdefsp.h>
#include <atlstr.h>
#include "vbscript.h"

#define SCRIPT_HOST_NAME	L"ActiveScript"

//const IID CLSID_VBScript = {0xb54f3741, 0x5b07, 0x11cf, {0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8 } };
//const IID CLSID_PerlScript = {0xF8D77580, 0x0F09, 0x11d0, {0xaa, 0x61, 0x3c, 0x28, 0x4e, 0x0, 0x0, 0x0 } };
//DEFINE_GUID(CLSID_VBScript, 0xb54f3741, 0x5b07, 0x11cf, 0xa4, 0xb0, 0x0, 0xaa, 0x0, 0x4a, 0x55, 0xe8);
//DEFINE_GUID(CLSID_JScript, 0xf414c260, 0x6ac0, 0x11cf, 0xb6, 0xd1, 0x00, 0xaa, 0x00, 0xbb, 0xbb, 0x58);

CVbScript::CVbScript(void)
{
	memset(&m_scriptId, 0, sizeof(m_scriptId));
	//m_scriptId = 0;
}

CVbScript::~CVbScript(void)
{
	Close();
}

void CVbScript::AddFunction(LPCWSTR szName, LPCWSTR aszArgs[], LPCWSTR szBody)
{
	if(m_pEngine != NULL)	{
		std::wstringstream wstr;
		if(m_scriptId == CLSID_VBScript)	{
			wstr << L"Function " << szName << L"(";
			for(int i=0;aszArgs[i];i++)	wstr << (i?L",":L"") << aszArgs[i];
			wstr << L")" << std::endl << szBody << std::endl << "End Function";
		}	else	{
			wstr << L"sub " << szName << L" {";
			wstr << std::endl << szBody << std::endl << "}";
		}
		EXCEPINFO   ei;
		m_pParser->ParseScriptText(wstr.str().c_str(), NULL, NULL, NULL, 0, 0, 0L, NULL, &ei);
	}
}

void CVbScript::AddVariable(LPCWSTR szName, const _variant_t& vValue)
{
	if(m_pEngine != NULL)	{
		EXCEPINFO   ei;
		std::wstringstream wstr;
		wstr << L"$" << szName << L" = \"" << (_bstr_t)vValue << L"\"";
		m_pParser->ParseScriptText(wstr.str().c_str(), NULL, NULL, NULL, 0, 0, 0L, NULL, &ei);
	}
}

bool CVbScript::Init(const IID scriptId)
{
	m_scriptId = scriptId;
	if( SUCCEEDED(m_pEngine.CreateInstance(scriptId)) &&
		SUCCEEDED(m_pEngine->SetScriptSite(this)))	{
		m_pEngine->SetScriptState(SCRIPTSTATE_INITIALIZED);
		m_pParser = m_pEngine;
		if(m_pParser != NULL && SUCCEEDED(m_pParser->InitNew()))	{
			m_pEngine->SetScriptState(SCRIPTSTATE_CONNECTED);
			return true;
		}
	}
	return false;
}

void CVbScript::Close()
{
	m_pParser = NULL;
	m_pEngine = NULL;
}

_variant_t CVbScript::Exec(_bstr_t sScript, bool bRunScript)
{
	_variant_t	vRes;
	EXCEPINFO   ei;
	HRESULT hr = 0;
	if(m_pParser == NULL || FAILED(hr = m_pParser->ParseScriptText(sScript, NULL, NULL, NULL, 0, 0, bRunScript ? 0 : SCRIPTTEXT_ISEXPRESSION, &vRes, &ei)))
		vRes = _variant_t(hr, VT_ERROR);
	return vRes;
}

STDMETHODIMP CVbScript::QueryInterface(REFIID iid, LPVOID* ppvObj)
{
	return (iid == IID_IUnknown || iid == IID_IActiveScriptSite) ? *ppvObj = this, S_OK : E_NOINTERFACE;
}

STDMETHODIMP CVbScript::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown **ppiunkItem, ITypeInfo **ppti)
{
	return TYPE_E_ELEMENTNOTFOUND;
}

STDMETHODIMP CVbScript::OnScriptError(IActiveScriptError *pse)
{
	CString   strError;
	CString	  strArrow;
	CString	  strDesc;
	CString	  strLine;

	EXCEPINFO ei;
	DWORD     dwSrcContext;
	ULONG     ulLine;
	LONG      ichError;
	BSTR      bstrLine = NULL;

	HRESULT   hr;

	pse->GetExceptionInfo(&ei);
	pse->GetSourcePosition(&dwSrcContext, &ulLine, &ichError);
	hr = pse->GetSourceLineText(&bstrLine);
	if (hr)
		hr = S_OK;  // Ignore this error, there may not be source available

	if (!hr)
	{
		strError=ei.bstrSource;

		strDesc=ei.bstrDescription;
		strLine=bstrLine;

		if (ichError > 0 && ichError < 255)
		{
			strArrow=CString(_T('-'),ichError);
			strArrow.SetAt(ichError-1,_T('v'));

		}

		CString strErrorCopy=strError;
		strError.Format(_T("Source:'%s'\nFile:'%s'  Line:%d  Char:%ld\nError:%d  '%s'\n%s\n%s"),
			LPCTSTR(strErrorCopy),
			_T("File"),
			ulLine,
			ichError,
			(int)ei.wCode,
			LPCTSTR(strDesc),
			LPCTSTR(strArrow),
			LPCTSTR(strLine));

		MessageBox(NULL, strError, TEXT("VBScript"), MB_OK);
	}

	if (bstrLine)
		SysFreeString(bstrLine);

	return hr;
}
