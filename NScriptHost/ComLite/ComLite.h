#pragma once

namespace com	{

// Basic policy-based IUnknown implementation
//
// available policies:
//   thread_model:	single_threaded: ++/-- counter operations
//					multi_threaded:  interlocked counter (default)
//					scoped:          counter is not used, lifetime is managed by owner. violates COM rules.
//
//   lock_policy:	no_lock:		 no module lock (default)
//                  module_lock:     use DLL-module lock (requires ClassFactory.h)
//
template<class thread_model = multi_thread, class lock_policy = no_lock>
class UnknownT : public IUnknown, protected thread_model
{
protected:
	enum EntryType {etOffset, etAggregate};
	typedef struct {IID iid; INT_PTR offset; EntryType type;} InterfaceEntry;
	template <class T> InterfaceEntry Interface(T* pt)			{InterfaceEntry it = {__uuidof(T), (INT_PTR)pt - (INT_PTR)this, etOffset}; return it;}
	template <class T> InterfaceEntry Aggregate(IUnknown*& pt)	{InterfaceEntry it = {__uuidof(T), (INT_PTR)&pt- (INT_PTR)this, etAggregate}; return it;}
	STDMETHODIMP_(ULONG) IntAddRef()			{return Increment();}
	STDMETHODIMP_(ULONG) IntRelease()			{ULONG c = Decrement(); return c ? c : (delete this, 0U);}
	STDMETHODIMP IntQueryInterface(REFIID iid, void ** ppvObj)	{
		if(!ppvObj)	return E_POINTER;
		*ppvObj = iid == IID_IUnknown ? GetUnknown() : GetInterface(iid);
		for(InterfaceEntry *p = GetInterfaceTable(); p && p->iid != GUID_NULL && !*ppvObj; p++)	{
			if(p->iid == iid)	{		
				switch(p->type)	{
				case etOffset:		*ppvObj = (PBYTE)this + p->offset; break;
				case etAggregate:	return (*reinterpret_cast<IUnknown**>((PBYTE)this + p->offset))->QueryInterface(iid, ppvObj);
				}
			}
		}
		return *ppvObj ? AddRef(), S_OK : E_NOINTERFACE;
	}
	UnknownT()									{lock_policy::Lock();}
	virtual ~UnknownT()							{lock_policy::Unlock();}
	// AddRef/Release wrapped version of the constructor that allows to create aggregates
	virtual void Construct()					{};
	// One of these two functions should be overwritten by derived object
	virtual InterfaceEntry *GetInterfaceTable()	{return nullptr;}
	virtual void *GetInterface(REFIID iid)		{return nullptr;}
public:
	typedef UnknownT<thread_model, lock_policy>	UnknownType;
	IUnknown* GetUnknown()						{return static_cast<IUnknown*>(this);}
};

struct no_lock			{
	static void Lock()	{}
	static void Unlock(){}
};

class multi_thread		{
	long volatile _counter;
protected:
	multi_thread() : _counter(0)	{}
	ULONG Increment()	{return InterlockedIncrement(&_counter);}
	ULONG Decrement()	{return InterlockedDecrement(&_counter);}
};

class single_thread		{
	long _counter;
protected:
	single_thread()	: _counter(0)	{}
	ULONG Increment()	{return ++_counter;}
	ULONG Decrement()	{return --_counter;}
};

class scoped			{
protected:
	ULONG Increment()	{return 42;}
	ULONG Decrement()	{return 42;}
};

typedef UnknownT<>	Unknown;

// Simple COM object implementation
template<class T> class Object : public T
{
public:
	Object(bool addref = false, IUnknown* pouter = nullptr)	{Increment(); Construct(); if(!addref)	Decrement();}
	STDMETHODIMP_(ULONG) AddRef()							{return IntAddRef(); }
	STDMETHODIMP_(ULONG) Release()							{return IntRelease();}
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)	{return IntQueryInterface(iid, ppvObj);}
	operator IUnknown*()									{return nullptr;}
};

// Aggregated COM object implementation (handles outer IUnknown interface)
#pragma warning(push)
#pragma warning(disable: 4355) // 'this' used in initializer list
template<class T> class ObjectAgg : public Object<typename T::UnknownType>
{
	typedef bool aggregatable;
public:
	ObjectAgg(bool addref = false, IUnknown* pouter = nullptr) : Object(addref), _contained(pouter ? pouter : this)	{}
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)	{
		return iid == IID_IUnknown ? IntQueryInterface(iid, ppvObj) : _contained.IntQueryInterface(iid, ppvObj);
	}

protected:
	template<class T> class Contained : public T	{
		friend class ObjectAgg;
		IUnknown*	_pouter;
	public:
		Contained(IUnknown* pouter)								{_pouter = pouter;}
		STDMETHODIMP_(ULONG) AddRef()							{return _pouter->AddRef();}
		STDMETHODIMP_(ULONG) Release()							{return _pouter->Release();}
		STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)	{return _pouter->QueryInterface(iid, ppvObj);}
	};
	Contained<T>	_contained;
};
#pragma warning(pop)

template<class T> T* CreateObject(bool addref = false, IUnknown *pouter = nullptr)	{return new T(addref, pouter);}

template<class T, class... ARGS> T* CreateAndInitObject(ARGS&&... args) {
	T *pobj = com::CreateObject<T>();
	if(pobj)	pobj->Init(std::forward<ARGS>(args)...);
	return pobj;
}

template<class T, class I, class... ARGS> bool CreateAndInitObject(I **pobj, ARGS&&... args) {
	if(nullptr == pobj || *pobj)			return false;
	if(nullptr == (*pobj = new T(true)))	return false;
	return (*pobj)->Init(std::forward<ARGS>(args)...);
}

IUnknownPtr CreateInstance(REFCLSID rclsId, LPCTSTR module = nullptr, IUnknown *pouter = nullptr)
{
	IUnknownPtr punk;
	LPFNGETCLASSOBJECT	pgco = nullptr;
	HMODULE hmod = module ? LoadLibrary(module) : GetModuleHandle(NULL);
	if(hmod && (pgco = (LPFNGETCLASSOBJECT)GetProcAddress(hmod, "DllGetClassObject"))) {
		IClassFactoryPtr pcf;
		if(S_OK == pgco(rclsId, IID_IClassFactory, (void**)&pcf)) {
			pcf->CreateInstance(pouter, IID_IUnknown, (void**)&punk);
		}
	}
	return punk;
};

}