// hash array extension object for using in NScript parser

#pragma once

namespace nscript	{

struct __declspec(uuid("{5204B44E-E352-4596-B073-C1A58EA88C8A}")) IHash : public IUnknown	{
	virtual HRESULT _stdcall GetValue(const tstring& key, variant_t& result) = 0;
	virtual HRESULT _stdcall SetValue(const tstring& key, const variant_t& value) = 0;
};

#define IID_IHash	__uuidof(IHash)
_COM_SMARTPTR_TYPEDEF(IHash, IID_IHash);

class Hash : public Object, public IHash	{
	class HashItem : public Object	{
		IHashPtr	_hash;
		tstring		_key;
	public:
		HashItem(IObjectPtr hash, tchar* key) : _hash(hash), _key(key)		{};
		HRESULT _stdcall Get(variant_t& result)								{return _hash->GetValue(_key, result);}
		HRESULT _stdcall Set(const variant_t& value)						{return _hash->SetValue(_key, value);}
		HRESULT _stdcall Call(const variant_t& params, variant_t& result)	{Get(result); return IObjectPtr(result)->Call(params, result);}
		HRESULT _stdcall Item(const variant_t& item, variant_t& result)		{Get(result); return IObjectPtr(result)->Item(item, result);}
		HRESULT _stdcall Index(const variant_t& index, variant_t& result)	{Get(result); return IObjectPtr(result)->Index(index, result);}
	};
public:
	unsigned long _stdcall AddRef()											{return Object::AddRef();}
	unsigned long _stdcall Release()										{return Object::Release();}
	HRESULT _stdcall QueryInterface(REFIID iid, void ** ppvObj)				{
		if (ppvObj && iid == IID_IHash)	{*ppvObj =  static_cast<IHash*>(this); AddRef(); return NOERROR;}	
		else return Object::QueryInterface(iid, ppvObj);
	}
	HRESULT _stdcall New(variant_t& result)									{result = static_cast<IHash*>(new Hash());   return S_OK;}
	HRESULT _stdcall Index(const variant_t& index, variant_t& result)		{result = new HashItem(this, (bstr_t)index); return S_OK;}
	HRESULT _stdcall Item(const variant_t& item, variant_t& result)			{result = new HashItem(this, (bstr_t)item); return S_OK;}
	HRESULT _stdcall GetValue(const tstring& key, variant_t& result)		{result = _hash[key]; return S_OK;}
	HRESULT _stdcall SetValue(const tstring& key, const variant_t& value)	{_hash[key] = value;  return S_OK;}
protected:
	std::map<tstring, variant_t> _hash;
};

}