#pragma once

#define	CLASS_FACTORY
#define CLASS_OBJECT2(cnt, obj, clsid, descr, model, progid)	static com::ClassObject<obj > Host##cnt (clsid, descr, model, progid);
#define CLASS_OBJECT1(cnt, obj, clsid, descr, model, progid)	CLASS_OBJECT2(cnt,obj, clsid, descr, model, progid)
#define CLASS_OBJECT(obj, clsid, descr, model, progid)	CLASS_OBJECT1(__COUNTER__,obj, clsid, descr, model, progid)
#define	TYPE_LIBRARY()											static com::TypeLibrary TypeLib;

/* 
Usage of CLASS_OBJECT macro:

typedef com::Object<HelloImp> Hello;
CLASS_OBJECT(Hello, CLSID_HELLO, TEXT("Hello tester"), com::tmApartment, TEXT("Hello.World"));

Notes: There is no marschalling implemented by default, so the inter-apartment calls 
will cause a failure if no custom marschalling is provided.
*/

namespace com	{

enum ThreadingModel	{tmNone, tmApartment, tmFree, tmBoth, tmNeutral, tmUnknown};

LONG  LockModule(BOOL lock);
HINSTANCE GetInstance();

struct module_lock	{
	static void Lock()		{LockModule(true);}
	static void Unlock()	{LockModule(false);}
};

class ClassFactory : public IClassFactory
{
public:
	ClassFactory(REFCLSID clsId, LPCTSTR description, ThreadingModel threadingModel, LPCTSTR progId);
	virtual ~ClassFactory()						{}
	STDMETHODIMP_(ULONG) AddRef()				{return LockModule(true),  42;}
	STDMETHODIMP_(ULONG) Release()				{return LockModule(false), 42;}
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)	{
		if (!ppvObj) return E_POINTER;
		if (iid == IID_IUnknown)			{*ppvObj =      static_cast<IUnknown*>(this); AddRef(); return S_OK;}
		if (iid == IID_IClassFactory)		{*ppvObj = static_cast<IClassFactory*>(this); AddRef(); return S_OK;}
		return *ppvObj = NULL, E_NOINTERFACE;
	}
	STDMETHODIMP LockServer(BOOL lock)			{return LockModule(lock), S_OK;}
	virtual STDMETHODIMP Register();
	virtual STDMETHODIMP Unregister();
	STDMETHODIMP GetFactory(REFCLSID rclsId, REFIID riid, void** ppvObj)	{
		return rclsId == _clsId ? QueryInterface(riid, ppvObj) : _next ? _next->GetFactory(rclsId, riid, ppvObj) : E_NOINTERFACE;
	}
	STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)	{return E_NOTIMPL;}
protected:
	ClassFactory*	_next;
	CLSID			_clsId;
	ThreadingModel	_threadingModel;
	LPCTSTR			_description;
	LPCTSTR			_progId;
};

template<class T> class ClassObject : public ClassFactory
{
public:
	ClassObject(REFCLSID clsId, LPCTSTR description, ThreadingModel threadingModel = tmBoth, LPCTSTR progId = NULL) :
		ClassFactory(clsId, description, threadingModel, progId)						{}
	STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)	{
		__if_not_exists(T::aggregatable)	{
			if(pUnkOuter)						return CLASS_E_NOAGGREGATION;
		}
		if(pUnkOuter && riid != IID_IUnknown)	return E_NOINTERFACE;
		T* t = CreateObject<T>(true, pUnkOuter);
		if(t == NULL)							return E_OUTOFMEMORY;
		HRESULT hr = t->QueryInterface(riid, ppvObject);
		t->Release();
		return hr;
	}
};

class TypeLibrary : public ClassFactory
{
public:
	TypeLibrary() : ClassFactory(IID_NULL, NULL, tmBoth, NULL)						{}
	STDMETHODIMP Register();
	STDMETHODIMP Unregister();
};

IUnknownPtr CreateInstance(REFCLSID rclsId, LPCTSTR module = NULL, IUnknown *pouter = NULL);

}