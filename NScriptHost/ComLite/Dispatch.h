#pragma once

namespace com	{

// Basic policy-based IDispatch implementation
//
// available policies:
//   typeinfo:		embedded:		use embedded typelib info (generated from IDL, ODL or attributes)
//					custom:			use client-provided dispatch map (default)
template<class I, class typeinfo = custom<I>>
class Dispatch : public I, protected typeinfo
{
public:
	// IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)	{
		return pctinfo ? *pctinfo = 1, S_OK : E_INVALIDARG;
	}
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)	{
		if(!pptinfo)				return E_INVALIDARG;
		if(itinfo)					return DISP_E_BADINDEX;
		*pptinfo = typeinfo::GetTypeInfo(lcid);
		return *pptinfo ? (*pptinfo)->AddRef(), S_OK : E_INVALIDARG;
	}
	STDMETHODIMP GetIDsOfNames(const IID &riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)	{
		return DispGetIDsOfNames(typeinfo::GetTypeInfo(lcid), rgszNames, cNames, rgdispid);
	}
	STDMETHODIMP Invoke(DISPID dispidMember, const IID &riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT *puArgErr)	{
		return DispInvoke(static_cast<I*>(this), typeinfo::GetTypeInfo(lcid), dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}
};

template<class I, unsigned MAX_ENTRIES = 32, unsigned MAX_PARAMS = 16> 
class custom	{
	ITypeInfoPtr	_typeInfo;
protected:
	struct DispatchEntry	{
		METHODDATA	method;
		PARAMDATA	params[MAX_PARAMS];
	};

	template <class I, typename F> unsigned vtbl_offset(F f)	{
		struct {
			virtual unsigned _stdcall get0()  { return 0; }
			virtual unsigned _stdcall get1()  { return 1; }
			virtual unsigned _stdcall get2()  { return 2; }
			virtual unsigned _stdcall get3()  { return 3; }
			virtual unsigned _stdcall get4()  { return 4; }
			virtual unsigned _stdcall get5()  { return 5; }
			virtual unsigned _stdcall get6()  { return 6; }
			virtual unsigned _stdcall get7()  { return 7; }
			virtual unsigned _stdcall get8()  { return 8; }
			virtual unsigned _stdcall get9()  { return 9; }
			virtual unsigned _stdcall get10() { return 10; }
			virtual unsigned _stdcall get11() { return 11; }
			virtual unsigned _stdcall get12() { return 12; }
			virtual unsigned _stdcall get13() { return 13; }
			virtual unsigned _stdcall get14() { return 14; }
			virtual unsigned _stdcall get15() { return 15; }
			// ... more ...
		} vt;

		I* t = reinterpret_cast<I*>(&vt);
		typedef unsigned (_stdcall I::*index_func)(); 
		index_func get_index = (index_func)f;
		return (t->*get_index)();
	}

	ITypeInfo* GetTypeInfo(LCID lcid)	{
		static INTERFACEDATA idata = {0};
		static METHODDATA mdata[MAX_ENTRIES];
		if(!_typeInfo)	{
			idata.cMembers = 0;
			LPMETHODDATA pm = mdata;
			for(DispatchEntry *pe = GetDispatchMap(); pe && pe->method.dispid; pe++, pm++)	{
				if(++idata.cMembers > MAX_ENTRIES)	break;
				*pm = pe->method;
				pm->ppdata = pe->params;
			}
			idata.pmethdata = mdata;
			CreateDispTypeInfo(&idata, lcid, &_typeInfo);
		}
		return _typeInfo;
	}

	template <typename F> DispatchEntry DispMethod(UINT type, DISPID id, LPOLESTR name, F func, VARTYPE resultType, ...)
	{
		va_list args;
		va_start(args, resultType);
		DispatchEntry di = {{name, NULL, id, vtbl_offset<I>(func), CC_STDCALL, 0, type, resultType}, {NULL, VT_EMPTY}};
		unsigned i;
		for(i=0; i<MAX_PARAMS; i++)	{
			di.params[i].szName = va_arg(args, LPOLESTR);
			if(!di.params[i].szName)	break;
			di.params[i].vt		= va_arg(args, VARTYPE);
		}
		di.method.cArgs = i;
		va_end(args);
		return di;
	}

	virtual DispatchEntry* GetDispatchMap()		{return NULL;}
};

template <class I> struct embedded		{
	ITypeInfoPtr	_typeInfo;
protected:
	ITypeInfo* GetTypeInfo(LCID lcid)	{
		if(!_typeInfo)	{
			WCHAR filename[MAX_PATH];
			ITypeLibPtr	typelib;
			if(GetModuleFileNameW(GetInstance(), filename, MAX_PATH) && 
				SUCCEEDED(LoadTypeLib(filename,  &typelib)))
				typelib->GetTypeInfoOfGuid(__uuidof(I), &_typeInfo);
		}
		return _typeInfo;
	}
};

}