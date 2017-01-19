// NDispatch - extension for NScript parser to support OLE Automation via IDispatch calls. 
// Now all you can do in VBScript, is also possible in NScript (with some restrictions, however).

#pragma once

#include "NScript.h"

namespace nscript	{

class DispatchItem : public Object
{
	STDMETHODIMP Invoke(UINT type, const variant_t& params, variant_t& result);
	STDMETHODIMP Wrap(HRESULT hr, variant_t& result);

	IDispatchPtr	_dispatch;
	DISPID			_dispid;
	variant_t		_index;
public:
	DispatchItem(IDispatch *pdispatch, DISPID dispid) : _dispatch(pdispatch), _dispid(dispid)	{}
	DispatchItem(IDispatch *pdispatch, DISPID dispid, const variant_t& index) : _dispatch(pdispatch), _dispid(dispid), _index(index)	{}
	STDMETHODIMP Get(variant_t& result)								{return _dispid < 0 ? Object::Get(result) : Wrap(Invoke(DISPATCH_PROPERTYGET, variant_t(), result), result);}
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{return Wrap(Invoke(DISPATCH_METHOD, params, result), result);}
	STDMETHODIMP Set(const variant_t& value)						{return Invoke(DISPATCH_PROPERTYPUT, value, variant_t());}
	STDMETHODIMP Index(const variant_t& index, variant_t& result)	{return result = new DispatchItem(_dispatch, _dispid, index), S_OK;}
	STDMETHODIMP Item(const variant_t& params, variant_t& result);

};

class CreateObject : public Object
{
public:
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{return result = new DispatchItem(IDispatchPtr((LPCTSTR)bstr_t(params)), -1), S_OK;}
};

}