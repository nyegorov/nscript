#include "stdafx.h"
#include "NDispatch.h"

namespace nscript	{

STDMETHODIMP DispatchItem::Wrap(HRESULT hr, variant_t& result)
{
	if(SUCCEEDED(hr) && V_VT(&result) == VT_DISPATCH)	result = new DispatchItem(result, -1);
	return hr;
}

STDMETHODIMP DispatchItem::Invoke(UINT type, const variant_t& params, variant_t& result)								
{
	EXCEPINFO excepInfo		= {0};
	DISPPARAMS dispparams	= {0};
	UINT uargerr			= -1;
	DISPID dispidNamed		= DISPID_PROPERTYPUT;

	if(type == DISPATCH_PROPERTYPUT)	{
		dispparams.cNamedArgs		= 1;
		dispparams.rgdispidNamedArgs= &dispidNamed;
		dispparams.cArgs			= 1;
		dispparams.rgvarg			= &const_cast<variant_t&>(params);
	}	else if(type == DISPATCH_METHOD)	{
		SafeArray a(const_cast<variant_t&>(params));
		dispparams.cArgs			= a.Count();
		dispparams.rgvarg			= a.GetData();
		dispparams.cNamedArgs		= 0;	//dispparams.cArgs;
		dispparams.rgdispidNamedArgs= NULL;	//(DISPID*)alloca(dispparams.cArgs * sizeof(DISPID));
		//for(unsigned i=0; i<dispparams.cArgs; i++)	dispparams.rgdispidNamedArgs[i] = i;
	}

	if(V_VT(&_index) != VT_EMPTY)	{
		// indexed property
		SafeArray ai(_index);
		VARIANT *p = (VARIANT*)_malloca((dispparams.cArgs + ai.Count()) * sizeof(VARIANT));
		CopyMemory(p, dispparams.rgvarg, dispparams.cArgs * sizeof(VARIANT));
		CopyMemory(p + dispparams.cArgs, ai.GetData(), ai.Count() * sizeof(VARIANT));
		dispparams.rgvarg =	p;
		dispparams.cArgs += ai.Count();
	}

	HRESULT hr = _dispatch->Invoke(_dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, type, &dispparams, &result, &excepInfo, &uargerr);

	return hr;
}

STDMETHODIMP DispatchItem::Item(const variant_t& params, variant_t& result)								
{
	if(_dispid >= 0)	{
		Invoke(DISPATCH_PROPERTYGET, variant_t(), result);
		_dispatch = result;
	}
	DISPID dispid = -1;
	LPOLESTR name = bstr_t(params);
	HRESULT hr = _dispatch->GetIDsOfNames(IID_NULL, &name, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
	if(SUCCEEDED(hr)) result = new DispatchItem(_dispatch, dispid);
	return hr;
}

}