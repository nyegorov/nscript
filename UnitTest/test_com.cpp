#include "pch.h"
#include "CppUnitTest.h"

#include "../NScriptHost/NScript/NScript.h"
#include "../NScriptHost/NScript/NHash.h"
#include "../NScriptHost/NScript/NDispatch.h"

#include "../NScriptHost/ComLite/ComLite.h"
#include "../NScriptHost/ComLite/ClassFactory.h"

#import "../NScriptHost/NScriptHost.tlb" no_namespace named_guids

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

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

namespace UnitTest
{
	TEST_CLASS(ComLiteTest)
	{
	public:
		TEST_CLASS_INITIALIZE(Init)
		{
			CoInitialize(NULL);
		}

		TEST_CLASS_CLEANUP(Cleanup)
		{
			CoUninitialize();
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

		TEST_METHOD(Aggregation)
		{

			TestAggregation(com::CreateObject<com::Object<CatDogAggregated>>());
			TestAggregation(com::CreateObject<com::Object<CatDogContained>>());
		}

		TEST_METHOD(Loading)
		{
			// Create local object
			ICatPtr cat = com::CreateObject<com::Object<CatImp>>();
			Assert::IsNotNull<ICat>(cat);
			Assert::IsNull<IDog>(IDogPtr(cat));
			Assert::AreEqual(S_OK, cat->Meow());

			// Create remote object
			INScriptPtr ns = com::CreateInstance(CLSID_Parser, TEXT("NScriptHost.dll"));
			Assert::IsNotNull<INScript>(ns);
			Assert::AreEqual(4L, (long)ns->Exec(L"2*2"));

		}
	};
}