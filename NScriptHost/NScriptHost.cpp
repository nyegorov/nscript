// NScriptHost.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "ComLite/ClassFactory.h"
#include "ComLite/ComLite.h"
#include "ComLite/Dispatch.h"
#include "NScript/NScript.h"
#include "NScript/NDispatch.h"

[module(dll, name = "NScript", uuid = "{CC27330C-FA87-4bdd-A2F2-8495DCBFCF1F}")];

[object, dual, uuid = "{4C0DBEF9-DF57-455a-B119-10E4D7744E55}"]
__interface INScript : public IDispatch {
	[id(1)] HRESULT AddObject([in] BSTR name, [in] VARIANT object);
	[id(2)] HRESULT Exec([in] BSTR script, [out, retval] VARIANT *pret);
};

[object, uuid = "{BF4F1403-B407-42aa-B642-F57B33FED8A1}"]
__interface IObject : public IUnknown {
	HRESULT New([out] VARIANT& result);
	HRESULT Get([out] VARIANT& result);
	HRESULT Set([in] const VARIANT& value);
	HRESULT Call([in] const VARIANT& params, [out] VARIANT& result);
	HRESULT Item([in] const VARIANT& item, [out] VARIANT& result);
	HRESULT Index([in] const VARIANT& index, [out] VARIANT& result);
};

typedef com::UnknownT<com::multi_thread, com::module_lock>	UnknownInproc;

struct ThreadId : public nscript::Object {
	STDMETHODIMP Call(const variant_t& params, variant_t& result) { return result = (long)GetCurrentThreadId(), S_OK; }
};

[coclass, uuid = "{3fd55c79-34ce-4ba1-b86d-e39da245cbca}"]
class Parser : public com::Dispatch<INScript, com::embedded<INScript>>, public ISupportErrorInfo, public UnknownInproc
{
	nscript::NScript		_script;
	IUnknownPtr				_ftm;
	bool					_inprocess;

	InterfaceEntry* GetInterfaceTable() {
		static InterfaceEntry table[] = {
			Interface<ISupportErrorInfo>(this),
			Interface<INScript>(this),
			Interface<IDispatch>(this),
			//			Aggregate<IMarshal>(_ftm.GetInterfacePtr()),
			NULL
		};
		return table;
	}

public:
	void Construct() {
		_inprocess = false;
		_script.AddObject(TEXT("thread_id"), new ThreadId());
		CoCreateFreeThreadedMarshaler(this->GetUnknown(), &_ftm);
	}
	STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid) { return (riid == __uuidof(INScript)) ? S_OK : S_FALSE; }

	HRESULT Exec(BSTR script, VARIANT *presult) {
		return _script.Exec(bstr_t(script), static_cast<variant_t&>(*presult));
	}
	HRESULT AddObject(BSTR name, VARIANT object) {
		if (V_VT(&object) == VT_DISPATCH) {
			V_UNKNOWN(&object) = IUnknownPtr(new nscript::DispatchItem(V_DISPATCH(&object), -1)).Detach();
			V_VT(&object) = VT_UNKNOWN;
		}
		return _script.AddObject(bstr_t(name), object);
	}
};

#define CLSID_NScript	__uuidof(Parser)

typedef com::Object<Parser>  ParserImp;

TYPE_LIBRARY();
CLASS_OBJECT(ParserImp, CLSID_NScript, TEXT("NScript parser"), com::tmBoth, TEXT("NScript"));

