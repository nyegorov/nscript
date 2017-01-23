#include "pch.h"
#include "CppUnitTest.h"

#include "VbScript.h"

#include "../NScriptHost/NScript/NScript.h"
#include "../NScriptHost/NScript/NHash.h"
#include "../NScriptHost/NScript/NDispatch.h"

#include "../NScriptHost/ComLite/ComLite.h"
#include "../NScriptHost/ComLite/ClassFactory.h"

#import "../NScriptHost/NScriptHost.tlb" no_namespace named_guids

const char vb_script[] = R"(
dim result

class hallo
	function hail(name1, name2)
		hail = name1 & " hails " & name2
	end function
end class

sub test_nscript
	dim ns, hal
	set ns = createobject("NScript")
	ns.addobject "name", "VBScript"
	ns.addobject "hallo", new hallo
	result = ns.exec("hallo.hail(name, 'NScript') + '!'")
end sub

test_nscript
)";

const wchar_t apt_test[] = L"str(thread_id()) + ' -> ' + str(callback_id())";

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

struct ThreadId : public nscript::Object {
	STDMETHODIMP Call(const variant_t& params, variant_t& result) { return result = (long)GetCurrentThreadId(), S_OK; }
};

struct __declspec(uuid("{1FD0A6FB-AFEB-47b7-AC31-879D4958D2BC}")) ICat : public IUnknown {
	virtual HRESULT Meow() = 0;
};

struct __declspec(uuid("{1FD0A6FB-AFEB-47b7-AC31-879D4958D2BD}")) IDog : public IUnknown {
	virtual HRESULT Bark() = 0;
};

#define IID_ICat	__uuidof(ICat)
#define IID_IDog	__uuidof(IDog)
_COM_SMARTPTR_TYPEDEF(ICat, IID_ICat);
_COM_SMARTPTR_TYPEDEF(IDog, IID_IDog);

class CatImp : public com::Unknown, public ICat
{
	void* GetInterface(REFIID iid) {return iid == IID_ICat ? static_cast<ICat*>(this) : NULL;}
public:
	HRESULT Meow() { return S_OK; }
};

class DogImp : public com::Unknown, public IDog
{
	void* GetInterface(REFIID iid) { return iid == IID_IDog? static_cast<IDog*>(this) : NULL; }
public:
	HRESULT Bark() { return S_OK; }
};

class CatDogAggregated : public com::Unknown
{
	IUnknownPtr	_cat;
	IUnknownPtr	_dog;
	void* GetInterface(REFIID iid) {
		IUnknown *pobj;
		if (iid == IID_ICat && SUCCEEDED(_cat.QueryInterface(iid, &pobj)))	return pobj->Release(), pobj;
		if (iid == IID_IDog && SUCCEEDED(_dog.QueryInterface(iid, &pobj)))	return pobj->Release(), pobj;
		return NULL;
	}
public:
	virtual void Construct() {
		_cat = com::CreateObject<com::ObjectAgg<CatImp>>(false, this);
		_dog = com::CreateObject<com::ObjectAgg<DogImp>>(false, this);
	}
};

class CatDogAggregated2 : public com::Unknown
{
	IUnknownPtr	_cat;
	IUnknownPtr	_dog;
	InterfaceEntry* GetInterfaceTable() {
		static InterfaceEntry table[] = {
			Aggregate<ICat>(_cat.GetInterfacePtr()),
			Aggregate<IDog>(_dog.GetInterfacePtr()),
			NULL
		};
		return table;
	}
public:
	virtual void Construct() {
		_cat = com::CreateObject<com::ObjectAgg<CatImp>>(false, this);
		_dog = com::CreateObject<com::ObjectAgg<DogImp>>(false, this);
	}
};

class CatDogContained : public com::Unknown, public ICat, public IDog
{
	ICatPtr	_cat;
	IDogPtr	_dog;
	void* GetInterface(REFIID iid) {
		if (iid == IID_ICat) return static_cast<ICat*>(this);
		if (iid == IID_IDog) return static_cast<IDog*>(this);
		return NULL;
	}
public:
	HRESULT Meow() { return _cat->Meow(); }
	HRESULT Bark() { return _dog->Bark(); }
	virtual void Construct() {
		_cat = com::CreateObject<com::Object<CatImp>>();
		_dog = com::CreateObject<com::Object<DogImp>>();
	}
};

HRESULT RegisterLibrary(LPCTSTR module)
{
	HMODULE hmod = LoadLibrary(module);
	typedef HRESULT (STDAPICALLTYPE * LPFNREGISTERSERVER) (void);
	LPFNREGISTERSERVER prs = NULL;
	if(hmod && (prs = (LPFNREGISTERSERVER)GetProcAddress(hmod, "DllRegisterServer")))
		return prs();
	return E_FAIL;
}

HRESULT UnregisterLibrary(LPCTSTR module)
{
	HMODULE hmod = LoadLibrary(module);
	typedef HRESULT(STDAPICALLTYPE * LPFNUNREGISTERSERVER) (void);
	LPFNUNREGISTERSERVER prs = NULL;
	if (hmod && (prs = (LPFNUNREGISTERSERVER)GetProcAddress(hmod, "DllUnregisterServer")))
		return prs();
	return E_FAIL;
}

namespace UnitTest
{
	TEST_CLASS(ComLiteTest)
	{
	public:
		TEST_CLASS_INITIALIZE(Init)
		{
			Assert::AreEqual(S_OK, RegisterLibrary(TEXT("NScriptHost.dll")));
			CoInitialize(NULL);
		}

		TEST_CLASS_CLEANUP(Cleanup)
		{
			CoUninitialize();
			Assert::AreEqual(S_OK, RegisterLibrary(TEXT("NScriptHost.dll")));
		}

		void TestCall(INScriptPtr ns)
		{
			Assert::IsNotNull<INScript>(ns);
			Assert::AreEqual(4L, (long)ns->Exec(L"2*2"));
		}

		void TestAggregation(IUnknownPtr punk)
		{
			Assert::IsNotNull<IUnknown>(punk);
			ICatPtr cat = punk;
			IDogPtr dog = punk;
			Assert::IsNotNull<ICat>(cat);
			Assert::IsNotNull<IDog>(dog);
			Assert::AreEqual(S_OK, cat->Meow());
			Assert::AreEqual(S_OK, dog->Bark());

			Assert::IsNotNull<ICat>(ICatPtr(dog));
			Assert::IsNotNull<IDog>(IDogPtr(cat));					// symmetry
			Assert::IsNotNull<ICat>(ICatPtr(cat));					// reflectivity
			Assert::IsTrue(ICatPtr(dog) == cat);					// transitivity
			Assert::IsTrue(IUnknownPtr(dog) == IUnknownPtr(cat));	// identity
			Assert::IsNull<IDispatch>(IDispatchPtr(punk));
		}

		TEST_METHOD(Loading)
		{
			// Create local object
			ICatPtr cat = com::CreateObject<com::Object<CatImp>>();
			Assert::IsNotNull<ICat>(cat);
			Assert::IsNull<IDog>(IDogPtr(cat));
			Assert::AreEqual(S_OK, cat->Meow());

			// Create remote object
			TestCall(com::CreateInstance(CLSID_Parser, TEXT("NScriptHost.dll")));

			// COM 
			TestCall(INScriptPtr("NScript"));
			TestCall(INScriptPtr(CLSID_Parser));
		}

		TEST_METHOD(Aggregation)
		{

			TestAggregation(com::CreateObject<com::Object<CatDogAggregated>>());
			TestAggregation(com::CreateObject<com::Object<CatDogAggregated2>>());
			TestAggregation(com::CreateObject<com::Object<CatDogContained>>());
		}

		TEST_METHOD(Dispatch)
		{
			CVbScript vbs;
			vbs.Init(CLSID_VBScript);
			vbs.Exec(vb_script, true);
			variant_t res = vbs.Exec("result");
			Assert::AreEqual((LPCWSTR)bstr_t(res), L"NScript hails VBScript!");
		}

		TEST_METHOD(TypeLib)
		{
			variant_t res;
			INScriptPtr ns;
			ITypeLibPtr typelib;
			ITypeInfoPtr typeinfo;
			HRESULT hr = LoadRegTypeLib(LIBID_NScript, 1, 0, LOCALE_SYSTEM_DEFAULT, &typelib);
			Assert::AreEqual(S_OK, hr);
			hr = typelib->GetTypeInfoOfGuid(CLSID_Parser, &typeinfo);
			Assert::AreEqual(S_OK, hr);
			hr = typeinfo->CreateInstance(NULL, IID_INScript, (PVOID*)&ns);
			res = ns->Exec(L"'Hallo, TypeLib'");
			Assert::AreEqual((LPCWSTR)bstr_t(res), L"Hallo, TypeLib");
		}

		TEST_METHOD(ProxyStub)
		{
			LPSTREAM pstream = NULL;
			variant_t res;
			Assert::AreEqual(S_OK, CoMarshalInterThreadInterfaceInStream(IID_INScript, INScriptPtr(CLSID_Parser), &pstream));

			std::thread t([pstream, &res]() {
				CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
				INScriptPtr ns;
				CoGetInterfaceAndReleaseStream(pstream, IID_INScript, (PVOID*)&ns);
				res = ns->Exec("2*2");
				ns = NULL;
				CoUninitialize();
			});

			MSG msg;
			HANDLE handle = t.native_handle();
			while (MsgWaitForMultipleObjectsEx(1, &handle, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) != WAIT_OBJECT_0) {
				GetMessage(&msg, NULL, 0, 0);
				DispatchMessage(&msg);
			}
			t.join();
			Assert::AreEqual(4l, (long)res);
		}

		TEST_METHOD(Apartments)
		{
			LPSTREAM pstreams[3];
			string res[4];
			INScriptPtr ns(CLSID_Parser);
			ostrstream os;
			ns->AddObject(L"callback_id", new ThreadId());

			auto thread_func = [](string &res, LPSTREAM pstream, string name, DWORD ctx) {
				CoInitializeEx(NULL, ctx);
				INScriptPtr ns;
				ostrstream os;
				CoGetInterfaceAndReleaseStream(pstream, IID_INScript, (PVOID*)&ns);
				os << name << ": " << GetCurrentThreadId() << " -> " << bstr_t(ns->Exec(apt_test));
				res.assign(os.str(), (unsigned)os.pcount());
				ns = NULL;
				CoUninitialize();
			};

			for(int i=0; i<3; i++)	Assert::AreEqual(S_OK, CoMarshalInterThreadInterfaceInStream(IID_INScript, ns, &pstreams[i]));
			std::thread sta( thread_func, ref(res[0]), pstreams[0], "STA",  COINIT_APARTMENTTHREADED);
			std::thread mta1(thread_func, ref(res[1]), pstreams[1], "MTA1", COINIT_MULTITHREADED);
			std::thread mta2(thread_func, ref(res[2]), pstreams[2], "MTA2", COINIT_MULTITHREADED);
			os << "MAIN STA: " << GetCurrentThreadId() << " -> " << bstr_t(ns->Exec(apt_test));
			res[3].assign(os.str(), (unsigned)os.pcount());

			MSG msg;
			HANDLE handles[3] = { sta.native_handle(), mta1.native_handle(), mta2.native_handle() };
			DWORD ret, count = 0;
			do {
				ret = MsgWaitForMultipleObjectsEx(3, handles, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
				if (ret < WAIT_OBJECT_0 + 3) {
					count++;
				} else if (ret < WAIT_OBJECT_0 + 6) {
					GetMessage(&msg, NULL, 0, 0);
					DispatchMessage(&msg);
				}
			} while (count < 3);
			sta.join();
			mta1.join();
			mta2.join();
			for (int i = 0; i < 4; i++) {
				Logger::WriteMessage(res[i].c_str());
			}
		}
	};
}