#include "stdafx.h"
#include <locale>
#include <limits>
#include <algorithm>
#include <functional>
#include <tuple>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "NScript3.h"

#undef min
#undef max

namespace nscript3	{

const std::error_category& nscript_category()
{
	static nscript_category_impl instance;
	return instance;
}

std::error_code make_error_code(nscript_error e)			{ return std::error_code(static_cast<int>(e), nscript_category()); }
std::error_condition make_error_condition(nscript_error e)	{ return std::error_condition(static_cast<int>(e), nscript_category()); }


// Globals
/*
const TCHAR *Messages[] = {
#ifdef UNICODE
	TEXT("%S"),
#else
	TEXT("%s"),
#endif
	TEXT("Unexpected end"),
	TEXT("Missing '%c' character"),
	TEXT("Unknown variable '%s'"),
	TEXT("Too many iterations"),
	TEXT("Syntax error"),
};

// set ErrorInfo 
void SetError(HRESULT hr, const void* param = TEXT(""), Parser* parser = NULL)
{
	ICreateErrorInfoPtr pcei;
	CreateErrorInfo(&pcei);

	tchar buf[1024];
	if(HRESULT_FACILITY(hr) == FACILITY_NSCRIPT && HRESULT_CODE(hr) <= _countof(Messages))	{
		wsprintf(buf, Messages[HRESULT_CODE(hr)], param);
	}	else	{
		lstrcpyn(buf, (LPCTSTR)param, _countof(buf));
	}
	pcei->SetDescription(bstr_t(buf));
	if(parser)	{
		pcei->SetHelpContext((DWORD)parser->GetState());
		pcei->SetHelpFile(bstr_t(parser->GetContent(0,-1).c_str()));
	}

	SetErrorInfo(0, IErrorInfoPtr(pcei));
}
*/
// throw exception when hr fails
/*void check(std::error_code err, const void* param = nullptr, parser* parser = nullptr)	{
	throw err;
}

void check(std::errc err, const void* param = nullptr, parser* parser = nullptr) {
	check(make_error_code(err), param, parser);
}*/
/*
// process list of argument
HRESULT ProcessArgsList(const args_list& args, const variant_t& params, NScript& script)	{
	if(args.size() == 0)	{
		script.AddObject(TEXT("@"),params);
	}	else	{
		SafeArray a(const_cast<variant_t&>(params));
		if(args.size() != a.Count())	return DISP_E_BADPARAMCOUNT;
		const variant_t *pargs = a.GetData();
		for(int i = a.Count()-1; i>=0; i--)	script.AddObject(args[i].c_str(),pargs[i]);
	}
	return S_OK;
}
*/
// 'dereference' object. If v holds an ext. object, replace it with the value of object
value_t& operator *(value_t& v)	{
	if(auto pobj = std::get_if<object_ptr>(&v)) {
		v = (*pobj)->get();
	}
	return v;
}
/*
// is v an object?
IObjectPtr GetObject(const variant_t& v)	{
	IObjectPtr pobj;
	return V_VT(&v) == VT_UNKNOWN && SUCCEEDED(V_UNKNOWN(&v)->QueryInterface(IID_IObject, reinterpret_cast<void **>(&pobj))) ? pobj : NULL;
}

// SafeArray

SafeArray::~SafeArray()				{
	while(_count--)	SafeArrayUnaccessData(V_ARRAY(&_sa));
}

void SafeArray::Redim(long size)
{
	assert(_count == 0);
	SAFEARRAYBOUND sab = {(ULONG)size,0};
	if(!IsArray())	{
		long i = 0;
		variant_t v = _sa;
		_sa.Clear();
		V_VT(&_sa) = VT_VARIANT|VT_ARRAY;
		V_ARRAY(&_sa) = SafeArrayCreate(VT_VARIANT, 1, &sab);
		if(V_ARRAY(&_sa) == NULL)	Check(E_OUTOFMEMORY);
#pragma warning(suppress: 6387)
		Check(SafeArrayPutElement(V_ARRAY(&_sa), &i, (void*)&v));	
	}	else	{
		Check(SafeArrayRedim(V_ARRAY(&_sa), &sab));
	}
}

void SafeArray::Put(long index, const variant_t& value, bool forceArray)
{
	if(!IsArray() && index == 0 && !forceArray)	_sa = value;
	else	{
		if(_count)	Check(DISP_E_ARRAYISLOCKED);
		if(index >= Count())	Redim(index+1);
		Check(SafeArrayPutElement(V_ARRAY(&_sa), &index, (void*)&value));	
	}
}

void SafeArray::Get(long index, variant_t& value) const	{
	value.Clear();
	if(IsArray())	Check(SafeArrayGetElement(V_ARRAY(&_sa), &index, &value));
	else if(index == 0 && !IsEmpty())	value = _sa;
	else			Check(DISP_E_BADINDEX);
};

void SafeArray::Remove(long index)
{
	if(index < 0 || index >= Count())	Check(DISP_E_BADINDEX);
	if(!IsArray())	_sa = variant_t();
	else if(Count() == 2) { 
		index = 1 - index; 	
		variant_t tmp;
		Check(SafeArrayGetElement(V_ARRAY(&_sa), &index, &tmp));
		_sa = tmp;
	}	else {
		for(long i = index + 1; i < Count(); i++)	Put(i - 1, (*this)[i]);
		Redim(Count() - 1);
	}
}

void SafeArray::Append(const variant_t& value)
{
	SafeArray right(const_cast<variant_t&>(value));
	for(long i = 0; i < right.Count(); i++)	Add(right[i]);
}

long SafeArray::Count() const		{
	long ubound = 0;
	if(IsEmpty())		ubound = -1;
	else if(IsArray())	Check(SafeArrayGetUBound(V_ARRAY(&_sa), 1, &ubound));
	return ubound+1;
}
variant_t* SafeArray::GetData()	{
	variant_t *p = &_sa;
	if(IsArray())		{Check(SafeArrayAccessData(V_ARRAY(&_sa), (void**)&p)); _count++;}
	return p;
}

variant_t& SafeArray::operator [](long index)	{
	variant_t* p = &_sa;
	if(_count)	Check(DISP_E_ARRAYISLOCKED);
	if(index >= Count())	Redim(index+1);
	if(IsArray())		Check(SafeArrayPtrOfIndex(V_ARRAY(&_sa), &index, (void**)&p));
	return *p;
}

// Variable

STDMETHODIMP Variable::Index(const variant_t& index, variant_t& result)		{
	IObjectPtr pobj = GetObject(_value);
	return pobj ? 
		pobj->Index(index, result) : 
		(result = static_cast<IObject*>(new Indexer(this, index)), S_OK);
}

STDMETHODIMP Indexer::Get(variant_t& result)								{
	variant_t v(IUnknownPtr(_value).GetInterfacePtr());
	return SafeArray(*v).Get(_index, result), S_OK;
}

// Class

STDMETHODIMP Class::New(variant_t& result)	{
	Instance *pinst = new Class::Instance(_body.c_str(), &_context);
	result = pinst;
	return pinst->Init(_args, _params);
}

STDMETHODIMP Class::Instance::Init(const args_list& args, const variant_t& params)	{
	HRESULT hr = ProcessArgsList(args, params, _script);
	return FAILED(hr) ? hr : (_script.Parse(NScript::Script, variant_t(), false), S_OK);
}

STDMETHODIMP Class::Instance::Item(const variant_t& item, variant_t& result)	{
	return _script.Exec(bstr_t(item), result);
}

// Function

STDMETHODIMP Function::Call(const variant_t& params, variant_t& result)	{
	NScript script(_body.c_str(), &_context);
	HRESULT hr = ProcessArgsList(_args, params, script);
	return FAILED(hr) ? hr : script.Exec(NULL, result);
}
*/
// Operators

inline bool is_numeric(value_t v) { return v.index() < 2; }

enum class associativity { left, right };

struct op_base {
	const parser::token token = parser::token::end;
	const associativity assoc = associativity::left;
	template<class X, class Y> value_t operator()(X&& x, Y&& y) { throw nscript_error::syntax_error; return { 0 }; }
};

struct op_null : op_base { };

struct op_add : op_base {
	const parser::token token = parser::token::plus;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x + y }; }
	value_t operator()(int x, double y) { return { x + y }; }
	value_t operator()(double x, int y) { return { x + y }; }
	value_t operator()(double x, double y) { return { x + y }; }
};

struct op_sub : op_base {
	const parser::token token = parser::token::minus;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x - y }; }
	value_t operator()(int x, double y) { return { x - y }; }
	value_t operator()(double x, int y) { return { x - y }; }
	value_t operator()(double x, double y) { return { x - y }; }
};

struct op_mul : op_base {
	const parser::token token = parser::token::multiply;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x * y }; }
	value_t operator()(int x, double y) { return { x * y }; }
	value_t operator()(double x, int y) { return { x * y }; }
	value_t operator()(double x, double y) { return { x * y }; }
};

struct op_div : op_base {
	const parser::token token = parser::token::divide;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x / y }; }
	value_t operator()(int x, double y) { return { x / y }; }
	value_t operator()(double x, int y) { return { x / y }; }
	value_t operator()(double x, double y) { return { x / y }; }
};

struct op_pow : op_base {
	const parser::token token = parser::token::pwr;
	using op_base::operator();
	value_t operator()(int x, int y) { return { pow(x, y) }; }
	value_t operator()(int x, double y) { return { pow(x, y) }; }
	value_t operator()(double x, int y) { return { pow(x, y) }; }
	value_t operator()(double x, double y) { return { pow(x, y) }; }
};

/*void OpAnd(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarAnd(&*op1, &*op2, &result));}
void OpOr(variant_t& op1, variant_t& op2, variant_t& result)	{
	auto hr = VarOr(&*op1, &*op2, &result);
	if(FAILED(hr)) {
		IObjectPtr pobj = GetObject(op2);
		if(pobj)	Check(pobj->Call(*op1, result));
		return;
	}
	Check(hr);
}
void OpGT(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)==VARCMP_GT;}
void OpGE(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)>=VARCMP_EQ;}
void OpLT(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)==VARCMP_LT;}
void OpLE(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)<=VARCMP_EQ;}
void OpEqu(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)==VARCMP_EQ;}
void OpNeq(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)!=VARCMP_EQ;}
void OpLAnd(variant_t& op1, variant_t& op2, variant_t& result)	{result = (bool)*op1 && (bool)*op2;}
void OpLOr(variant_t& op1, variant_t& op2, variant_t& result)	{result = (bool)*op1 || (bool)*op2;}
void OpLNot(variant_t& op1, variant_t& op2, variant_t& result)	{result = !(bool)*op2;}
void OpNot(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarNot(&*op2, &result));}
void OpNeg(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarNeg(&*op2, &result));}
void OpNull(variant_t& op1, variant_t& op2, variant_t& result)	{}
void OpIndex(variant_t& op1, variant_t& op2, variant_t& result)	{
	IObjectPtr pobj = GetObject(op1);
	if(pobj)	Check(pobj->Index(*op2, result));	else	SafeArray(op1).Get(*op2, result);
	//if(pobj == NULL || FAILED(pobj->Index(*op2, result)))	SafeArray(op1).Get(*op2, result);
}
void OpJoin(variant_t& op1, variant_t& op2, variant_t& result)	{ SafeArray a(result = *op1); a.Append(*op2); }
void OpCall(variant_t& op1, variant_t& op2, variant_t& result)	{Check(IObjectPtr(op1)->Call(*op2, result));}
void OpAssign(variant_t& op1, variant_t& op2, variant_t& result){Check(IObjectPtr(op1)->Set(*op2));result = op1;}
void OpItem(variant_t& op1, variant_t& op2, variant_t& result)	{Check(IObjectPtr(op1)->Item(*op2, result)); }
void OpDot(variant_t& op1, variant_t& op2, variant_t& result)	{result = new Composer(*op1, *op2); }
void OpNew(variant_t& op1, variant_t& op2, variant_t& result)	{Check(IObjectPtr(op2)->New(result));}

template <void OP(variant_t& op1, variant_t& op2, variant_t& result)>
void OpXSet(variant_t& op1, variant_t& op2, variant_t& result)	{
	IObjectPtr pobj(op1);
	Check(pobj->Get(op1));
	OP(op1, op2, result);
	Check(pobj->Set(result));	
}
void OpPPX(variant_t& op1, variant_t& op2, variant_t& result)	{OpXSet<OpAdd>(op2,variant_t(1L),result);}
void OpMMX(variant_t& op1, variant_t& op2, variant_t& result)	{OpXSet<OpSub>(op2,variant_t(1L),result);}
void OpXPP(variant_t& op1, variant_t& op2, variant_t& result)	{variant_t v;result = op1;*result;OpXSet<OpAdd>(op1,variant_t(1L),v);}
void OpXMM(variant_t& op1, variant_t& op2, variant_t& result)	{variant_t v;result = op1;*result;OpXSet<OpSub>(op1,variant_t(1L),v);}
void OpHead(variant_t& op1, variant_t& op2, variant_t& result) { SafeArray a(*op2); a.Get(0, result); }
void OpTail(variant_t& op1, variant_t& op2, variant_t& result) { SafeArray a(result = *op1); a.Remove(0); }

// Functions

template<typename T, T FN(T)> void FnX(int n, const variant_t v[], variant_t& result)	{result = FN((T)v[0]);}
template<VARTYPE T, LCID lcid> void FnCvt(int n, const variant_t v[], variant_t& result)			{
	Check(VariantChangeTypeEx(&result, &v[0], lcid, VARIANT_ALPHABOOL, T));
}
// Date
void FnNow(int n, const variant_t v[], variant_t& result)		{SYSTEMTIME st;GetLocalTime(&st);SystemTimeToVariantTime(&st,&V_R8(&result));V_VT(&result) = VT_DATE;}
void FnDay(int n, const variant_t v[], variant_t& result)		{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wDay;}
void FnMonth(int n, const variant_t v[], variant_t& result)		{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wMonth;}
void FnYear(int n, const variant_t v[], variant_t& result)		{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wYear;}
void FnHour(int n, const variant_t v[], variant_t& result)		{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wHour;}
void FnMinute(int n, const variant_t v[], variant_t& result)	{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wMinute;}
void FnSecond(int n, const variant_t v[], variant_t& result)	{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wSecond;}
void FnDayOfWeek(int n, const variant_t v[], variant_t& result)	{SYSTEMTIME st;VariantTimeToSystemTime(V_DATE(&v[0]), &st);result = (long)st.wDayOfWeek;}
void FnDayOfYear(int n, const variant_t v[], variant_t& result)	{UDATE ud;VarUdateFromDate(V_DATE(&v[0]), 0, &ud);result = (long)ud.wDayOfYear;}
// Math
void FnPi(int n, const variant_t v[], variant_t& result)		{result  = (double)3.141592653589793238462643383279502884197169399375105820974944;}
void FnRnd(int n, const variant_t v[], variant_t& result)		{result  = (double)rand()/(double)RAND_MAX;}
void FnSgn(int n, const variant_t v[], variant_t& result)		{long l = v[0]; result = (l<0?-1L:l>0?1L:0L);}
void FnAtan2(int n, const variant_t v[], variant_t& result)		{result = atan2((double)v[0], (double)v[1]);}
void FnFract(int n, const variant_t v[], variant_t& result)		{result = (double)v[0] - (long)v[0];}
// String
void FnChr(int n, const variant_t v[], variant_t& result)		{tchar cc[2] = {(tchar)(long)v[0], 0};  result = cc;}
void FnAsc(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]); result = b.length() ? (long)*(tchar*)b : 0L;}
void FnLen(int n, const variant_t v[], variant_t& result)		{result = (long)((bstr_t)v[0]).length();}
void FnLeft(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]);result = b; SysReAllocStringLen(&V_BSTR(&result), b, (long)v[1]);}
void FnRight(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]);result = b; SysReAllocStringLen(&V_BSTR(&result), (LPOLESTR)b + b.length() - (long)v[1], (long)v[1]);}
void FnMid(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]);result = b; SysReAllocStringLen(&V_BSTR(&result), (LPOLESTR)b + (long)v[1], (long)v[2]);}
void FnUpper(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]);CharUpperBuff((tchar *)b, b.length());result = b;}
void FnLower(int n, const variant_t v[], variant_t& result)		{bstr_t b(v[0]);CharLowerBuff((tchar *)b, b.length());result = b;}
void FnString(int n, const variant_t v[], variant_t& result)	{bstr_t b = v[1]; result = tstring((long)v[0],*(tchar*)b).c_str();}
void FnReplace(int n, const variant_t v[], variant_t& result)	{
	tstring s((bstr_t)v[0]); 
	bstr_t from(v[1]), to(v[2]);
	tstring::size_type p = 0;
	for(;(p = s.find(from, p)) != tstring::npos;p+=to.length())	s.replace(p, from.length(), to, to.length());
	result = s.c_str();
}
void FnInstr(int n, const variant_t v[], variant_t& result)		{result = (long)tstring((bstr_t)v[0]).find((bstr_t)v[1]);}
void FnFormat(int n, const variant_t v[], variant_t& result)	{bstr_t b; VarFormat(&const_cast<variant_t&>(v[0]), (bstr_t)v[1], 0, 0, 0, &b.GetBSTR());result = b;}
void FnHex(int n, const variant_t v[], variant_t& result)		{
	FnCvt<VT_I4, LOCALE_INVARIANT>(n, v, result);
	TCHAR s[1024]; wsprintf(s, TEXT("%X"), V_I4(&result)); result = s;
}
// Array
void FnAdd(int n, const variant_t v[], variant_t& result)		{SafeArray(result=v[0]).Add(v[1]);}
void FnRemove(int n, const variant_t v[], variant_t& result)	{SafeArray(result = v[0]).Remove((long)v[1]);}

class FoldFunction : public Object
{
	IObjectPtr	_pfun;
public:
	FoldFunction(const variant_t& fun) : _pfun(GetObject(fun)) {}
	STDMETHODIMP Call(const variant_t& params, variant_t& result) {
		if(!_pfun)	return E_NS_SYNTAXERROR;
		variant_t vtmp;
		SafeArray src(const_cast<variant_t&>(params)), tmp(vtmp);
		result = src[0];
		for(long i = 1; i<src.Count(); i++) {
			tmp.Put(0, result);
			tmp.Put(1, src[i]);
			Check(_pfun->Call(vtmp, result));
		}
		return S_OK;
	}
};

class MapFunction : public Object
{
	IObjectPtr	_pfun;
public:
	MapFunction(const variant_t& fun) : _pfun(GetObject(fun)) {}
	STDMETHODIMP Call(const variant_t& params, variant_t& result) {
		if(!_pfun)	return E_NS_SYNTAXERROR;
		SafeArray src(const_cast<variant_t&>(params)), dst(result);
		dst.Redim(src.Count());
		for(long i = 0; i<src.Count(); i++) {
			variant_t tmp;
			Check(_pfun->Call(src[i], tmp));
			dst.Put(i, tmp);
		}
		return S_OK;
	}
};

class FilterFunction : public Object
{
	IObjectPtr	_pfun;
public:
	FilterFunction(const variant_t& fun) : _pfun(GetObject(fun)) {}
	STDMETHODIMP Call(const variant_t& params, variant_t& result) {
		if(!_pfun)	return E_NS_SYNTAXERROR;
		SafeArray src(const_cast<variant_t&>(params)), dst(result);
		for(long i = 0; i<src.Count(); i++) {
			variant_t tmp;
			Check(_pfun->Call(src[i], tmp));
			if((bool)*tmp)	dst.Add(src[i]);
		}
		return S_OK;
	}
};

void FnMap(int n, const variant_t v[], variant_t& result)  { result = new MapFunction(v[0]); }
void FnFold(int n, const variant_t v[], variant_t& result) { result = new FoldFunction(v[0]); }
void FnFilter(int n, const variant_t v[], variant_t& result) { result = new FilterFunction(v[0]); }
void FnHead(int n, const variant_t v[], variant_t& result) { result = v[0]; }
void FnTail(int n, const variant_t v[], variant_t& result) {
	if(n < 2)	return;
	SafeArray a(result);
	a.Redim(n - 1);
	for(int i = 1; i < n; i++)		a.Put(i-1, v[i]);
}

// Other
void FnSize(int n, const variant_t v[], variant_t& result)		{result = (long)n;}
void FnRgb(int n, const variant_t v[], variant_t& result)		{result = (long)RGB(v[0],v[1],v[2]);}
template<int cond>
void FnFind(int n, const variant_t args[], variant_t& result)	{
	if(!n)	{result.Clear(); return;}
	int idx = 0;
	for(int i=1;i<n;i++)	{
		if(VarCmp(&const_cast<variant_t&>(args[i]), &const_cast<variant_t&>(args[idx]), LOCALE_USER_DEFAULT, 0) == cond)	idx = i;
	}
	result = args[idx];
};
*/
// Context
struct copy_var {
	const context	*_from;
	context			*_to;
	copy_var(const context *from ,context *to) : _from(from), _to(to)	{};
	void operator() (const string_t& name) { /*_to->set(_from->get(name));*/ }
};
/*
Context::vars_t	Context::_globals = {
	{ TEXT("empty"),	variant_t()},
	{ TEXT("true"),		true},
	{ TEXT("false"),	false },
	{ TEXT("optional"),	variant_t(DISP_E_PARAMNOTFOUND, VT_ERROR) },
	// Conversion
	{ TEXT("int"),		new BuiltinFunction(1, FnCvt<VT_I4, LOCALE_INVARIANT>) },
	{ TEXT("dbl"),		new BuiltinFunction(1, FnCvt<VT_R8, LOCALE_INVARIANT>) },
	{ TEXT("str"),		new BuiltinFunction(1, FnCvt<VT_BSTR, LOCALE_USER_DEFAULT>) },
	{ TEXT("date"),		new BuiltinFunction(1, FnCvt<VT_DATE, LOCALE_USER_DEFAULT>) },
	// Date
	{ TEXT("now"),		new BuiltinFunction(0, FnNow) },
	{ TEXT("day"),		new BuiltinFunction(1, FnDay) },
	{ TEXT("month"),	new BuiltinFunction(1, FnMonth) },
	{ TEXT("year"),		new BuiltinFunction(1, FnYear) },
	{ TEXT("hour"),		new BuiltinFunction(1, FnHour) },
	{ TEXT("minute"),	new BuiltinFunction(1, FnMinute) },
	{ TEXT("second"),	new BuiltinFunction(1, FnSecond) },
	{ TEXT("dayofweek"),new BuiltinFunction(1, FnDayOfWeek) },
	{ TEXT("dayofyear"),new BuiltinFunction(1, FnDayOfYear) },
	// Math
	{ TEXT("pi"),		new BuiltinFunction(0, FnPi) },
	{ TEXT("rnd"),		new BuiltinFunction(0, FnRnd) },
	{ TEXT("sin"),		new BuiltinFunction(1, FnX<double,  sin>) },
	{ TEXT("cos"),		new BuiltinFunction(1, FnX<double,  cos>) },
	{ TEXT("tan"),		new BuiltinFunction(1, FnX<double,  tan>) },
	{ TEXT("atan"),		new BuiltinFunction(1, FnX<double, atan>) },
	{ TEXT("abs"),		new BuiltinFunction(1, FnX<double, fabs>) },
	{ TEXT("exp"),		new BuiltinFunction(1, FnX<double,  exp>) },
	{ TEXT("log"),		new BuiltinFunction(1, FnX<double,  log>) },
	{ TEXT("sqr"),		new BuiltinFunction(1, FnX<double, sqrt>) },
	{ TEXT("sqrt"),		new BuiltinFunction(1, FnX<double, sqrt>) },
	{ TEXT("atan2"),	new BuiltinFunction(2, FnAtan2) },
	{ TEXT("sgn"),		new BuiltinFunction(1, FnSgn) },
	{ TEXT("fract"),	new BuiltinFunction(1, FnFract) },
	// String
	{ TEXT("chr"),		new BuiltinFunction(1, FnChr) },
	{ TEXT("asc"),		new BuiltinFunction(1, FnAsc) },
	{ TEXT("len"),		new BuiltinFunction(1, FnLen) },
	{ TEXT("left"),		new BuiltinFunction(2, FnLeft) },
	{ TEXT("right"),	new BuiltinFunction(2, FnRight) },
	{ TEXT("mid"),		new BuiltinFunction(3, FnMid) },
	{ TEXT("upper"),	new BuiltinFunction(1, FnUpper) },
	{ TEXT("lower"),	new BuiltinFunction(1, FnLower) },
	{ TEXT("string"),	new BuiltinFunction(2, FnString) },
	{ TEXT("replace"),	new BuiltinFunction(3, FnReplace) },
	{ TEXT("instr"),	new BuiltinFunction(2, FnInstr) },
	{ TEXT("format"),	new BuiltinFunction(2, FnFormat) },
	{ TEXT("hex"),		new BuiltinFunction(1, FnHex) },
	// Array
	{ TEXT("add"),		new BuiltinFunction(2, FnAdd) },
	{ TEXT("remove"),	new BuiltinFunction(2, FnRemove) },
	{ TEXT("fold"),		new BuiltinFunction(1, FnFold) },
	{ TEXT("fmap"),		new BuiltinFunction(1, FnMap) },
	{ TEXT("filter"),	new BuiltinFunction(1, FnFilter) },
	{ TEXT("head"),		new BuiltinFunction(-1, FnHead) },
	{ TEXT("tail"),		new BuiltinFunction(-1, FnTail) },
	// Other
	{ TEXT("size"),		new BuiltinFunction(-1, FnSize) },
	{ TEXT("rgb"),		new BuiltinFunction(3,  FnRgb) },
	{ TEXT("min"),		new BuiltinFunction(-1, FnFind<VARCMP_LT>) },
	{ TEXT("max"),		new BuiltinFunction(-1, FnFind<VARCMP_GT>) },
};
*/
context::context(const context *base, const var_names *vars) : _locals(1)
{
	if(base) {
		if(vars)	std::for_each(vars->begin(), vars->end(), copy_var(base, this));
		else		_locals.assign(base->_locals.begin(), base->_locals.end());
		//_locals[0] = base->_locals[0];
	}
}
/*
variant_t& Context::Get(const tstring& name, bool local)
{
	if(!local)	{
		for(int i = (int)_locals.size() - 1; i >= -1; i--)	{
			vars_t& plane = i<0?_globals:_locals[i];
			vars_t::iterator p = plane.find(name);
			if(p != plane.end()) return	p->second;
		}
	}
	return _locals.back()[name] = static_cast<IObject*>(new Variable());
}

bool Context::Get(const tstring& name, variant_t& result) const
{
	for(std::vector<vars_t>::const_reverse_iterator ri = _locals.rbegin(); ri != _locals.rend(); ri++)	{
		vars_t::const_iterator p = ri->find(name);
		if(p != ri->end())	return result = p->second, true;
	}
	return false;
}
*/
// Operator precedence
/*	{{Parser::stmt,		&OpNull},	{Parser::end, NULL}},
	{{Parser::comma,	&OpNull},	{Parser::end, NULL}},
	{{Parser::assign,	&OpAssign},	{Parser::plusset, &OpXSet<OpAdd>}, {Parser::minusset, &OpXSet<OpSub>}, {Parser::mulset, &OpXSet<OpMul>}, {Parser::divset, &OpXSet<OpDiv>}, {Parser::idivset, &OpXSet<OpIDiv>}, { Parser::colon, &OpJoin }, {Parser::end, NULL}},
	{{Parser::ifop,		&OpNull},	{Parser::end, NULL}},
	{{Parser::land,		&OpLAnd},	{Parser::lor,	&OpLOr},	{Parser::end, NULL}},
	{{Parser::and,		&OpAnd},	{Parser::or,	&OpOr},		{Parser::end, NULL}},
	{{Parser::equ,		&OpEqu},	{Parser::nequ,	&OpNeq},	{Parser::end, NULL}},
	{{Parser::gt,		&OpGT},		{Parser::ge,	&OpGE},		{Parser::lt, &OpLT},	{Parser::le, &OpLE},	{Parser::end, NULL}},
	{{Parser::plus,		&OpAdd},	{Parser::minus,	&OpSub},	{Parser::end, NULL}},
	{{Parser::multiply,	&OpMul},	{Parser::divide,&OpDiv},	{Parser::idiv, &OpIDiv},{Parser::mod, &OpMod},	{Parser::end, NULL}},
	{{Parser::pwr,		&OpPow},	{Parser::mdot,	&OpDot},	{Parser::end, NULL}},
	{{Parser::unaryplus,&OpPPX},	{Parser::unaryminus,&OpMMX},{Parser::newobj,&OpNew},{Parser::not,	&OpNot},{Parser::lnot, &OpLNot},{Parser::minus, &OpNeg},{ Parser::apo, &OpHead }, {Parser::end, NULL}},
	{{Parser::unaryplus,&OpXPP},	{Parser::unaryminus,&OpXMM},{Parser::lpar, &OpCall},{Parser::dot, &OpItem}, {Parser::lsquare, &OpIndex},{ Parser::apo, &OpTail},{Parser::end, NULL}},
};*/

static auto s_operators = std::make_tuple(
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),
	std::tuple<>(),

	std::tuple<op_add, op_sub>(),
	std::tuple<op_mul, op_div>(),
	std::tuple<op_pow>(),
	std::tuple<>(),
	std::tuple<>()
);

value_t NScript::eval(std::string_view script, std::error_code& ec)
{
	value_t result;
	_context.push();
	try	{
		_parser.init(script);
		parse<Script>(result, false);
		if(_parser.get_token() != parser::end)	throw nscript_error::syntax_error;
		return *result;
	}
	catch(nscript_error e)	 { ec = e; }
	catch(std::error_code e) { ec = e; }
	_context.pop();
	return result;
}
/*
// Parse "if <cond> <true-part> [else <part>]" statement
void NScript::ParseIf(Precedence level, variant_t& result, bool skip)	{
	bool cond = skip || (bool)result;
	Parse(level, result, !cond || skip);
	if(_parser.GetToken() == Parser::ifelse || _parser.GetToken() == Parser::colon)	{
		_parser.Next();
		Parse(level, result, cond || skip);
	}
}

// Parse "for([<start>];<cond>;[<inc>])	<body>" statement
void NScript::ParseForLoop(variant_t& result, bool skip)	{
	Parser::State condition, increment, body;
	_parser.Next();
	if(_parser.GetToken() != Parser::lpar)		Check(E_NS_MISSINGCHARACTER, (void*)'(', &_parser);
	_parser.Next();
	if(_parser.GetToken() != Parser::stmt)	{	// start expression
		Parse(Statement, result, skip);
		if(_parser.GetToken() != Parser::stmt)	Check(E_NS_MISSINGCHARACTER, (void*)';', &_parser);
	}
	_parser.Next();
	condition = _parser.GetState();
	if(_parser.GetToken() != Parser::stmt)	{	// exit condition
		Parse(Statement, result, true);
		if(_parser.GetToken() != Parser::stmt)	Check(E_NS_MISSINGCHARACTER, (void*)';', &_parser);
	}
	_parser.Next();
	increment = _parser.GetState();
	if(_parser.GetToken() != Parser::rpar)	{	// increment
		Parse(Statement, result, true);
		if(_parser.GetToken() != Parser::rpar)	Check(E_NS_MISSINGCHARACTER, (void*)')', &_parser);
	}
	_parser.Next();
	body = _parser.GetState();
	Parse(Statement, result, true);				// body
	if(!skip)	{
		ULONGLONG start = GetTickCount64();
		while(true)	{
			Parse(Statement, condition, result);
			if(!(bool)result)	break;
			Parse(Statement, body, result);
			Parse(Statement, increment, result);
			if(GetTickCount64()-start > 5000)	Check(E_NS_TOOMANYITERATIONS);
		}
	}
}

// Parse comma-separated arguments list
void NScript::ParseArgList(args_list& args, bool forceArgs) {
	auto token = _parser.Next();
	if(token != Parser::lpar && !forceArgs) return;		// function without parameters
	if(token == Parser::lpar)	_parser.Next();

	while(_parser.GetToken() == Parser::name) {
		args.push_back(_parser.GetName());
		if(_parser.Next() != Parser::comma) break;
		_parser.Next();
	};
	if(args.size() == 1 && args.front() == TEXT("@"))	args.clear();

	if(_parser.GetToken() == Parser::rpar)	_parser.Next();
}

// Parse "sub [(<arguments>)] <body>" statement
void NScript::ParseFunc(variant_t& result, bool skip)
{
	args_list args;
	ParseArgList(args, _parser.GetToken() == Parser::idiv);
	Parser::State state = _parser.GetState();
	if(_parser.GetToken() == Parser::end)	Check(E_NS_SYNTAXERROR, NULL, &_parser);
	// _varnames contains list of variables to be captured by function
	if(!skip)	_varnames.clear();
	Parse(Assignment,result,true);
	if(!skip)	result = new Function(args, _parser.GetContent(state, _parser.GetState()).c_str(), &Context(&_context, &_varnames));
}

// Parse "object [(<arguments>)] {<body>}" statement
void NScript::ParseObj(variant_t& result, bool skip)
{
	args_list args;
	ParseArgList(args, false);
	if(_parser.GetToken() == Parser::lcurly)	_parser.Next();
	Parser::State state = _parser.GetState();
	if(_parser.GetToken() == Parser::end)	Check(E_NS_SYNTAXERROR, NULL, &_parser);
	// _varnames contains list of variables to be captured by object
	if(!skip)	_varnames.clear();
	Parse(Script,result,true);
	if(!skip)	result = new Class(args, _parser.GetContent(state, _parser.GetState()).c_str(), &Context(&_context, &_varnames));
	if(_parser.GetToken() == Parser::rcurly)	_parser.Next();
}

// Parse "var[:=]" statement
void NScript::ParseVar(variant_t& result, bool local, bool skip)
{
	tstring name = _parser.GetName();
	if(skip)	_varnames.insert(name);
	_parser.Next();
	if(_parser.GetToken() == Parser::setvar)	{
		_parser.Next(); 
		Parse(Assignment, result, skip); 
		if(!skip)	_context.Set(name, result);
	}	else	{
		if(!skip)	result = _context.Get(name, local);
	}
}

// Jump to position <state>, parse expression and return back
void NScript::Parse(Precedence level, Parser::State state, variant_t& result)
{
	Parser::State current = _parser.GetState();
	_parser.SetState(state);
	Parse(level, result, false);
	_parser.SetState(current);
}
*/

template <NScript::Precedence level, class OP> bool NScript::apply_op(OP op, value_t& result, bool skip)
{
	if(op.token == _parser.get_token()) {
		if(op.token != parser::lpar && op.token != parser::lsquare && op.token != parser::dot)	_parser.next();
		if(op.token == parser::ifop) {		// ternary "a?b:c" operator
										//ParseIf(Logical, result, skip);
			return true;
		}

		value_t left{ result }, right{ 0 };
		if(!skip && level != Script)	result = 0;

		// parse right-hand operand
		if(op.token == parser::dot) { right = _parser.get_value(); _parser.next(); }	// special case for '.' operator
		else if(level == Assignment || level == Unary)	parse<level>(right, skip);		// right-associative operators
		else if(op.token != parser::unaryplus && op.token != parser::unaryminus  && op.token != parser::apo && !(level == Script && _parser.get_token() == parser::rcurly))
			parse<level+1>(level == Script ? result : right, skip);					// left-associative operators
																					
		if(!skip)	result = std::visit(op, left, right);								// perform operator's action

		if(level == Unary)	return false;
		return true;
	}
	return false;
}


template <NScript::Precedence level> void NScript::parse(value_t& result, bool skip)
{
	// main parse loop
	parser::token token = _parser.get_token();
	// all other
	bool noop = true;

	// parse left-hand operand (for binary operators)
	if(level != Unary)	parse<(Precedence)(level + 1)>(result, skip);
	if(_parser.get_token() == parser::end)	return;

	while(std::apply([&](auto ...op) { return (apply_op<level, decltype(op)>(op, result, skip) || ...); }, std::get<level>(s_operators))) noop = false;

	// for unary operators, return right-hand expression if no operators were found
	if(level == Unary && noop)	parse<level+1>(result, skip);
}

template<> void NScript::parse<NScript::Statement>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	parse<Assignment>(result, skip);
	if(_parser.get_token() == parser::comma) {
		/*			value_t v = result;
		SafeArray a(result);
		result.Clear();
		a.Put(0, *v, true);
		do	{
		_parser.Next();
		Parse((Precedence)(level+1), v, skip);
		a.Add(*v);
		}	while(_parser.GetToken() == Parser::comma);*/
	}
}

template<> void NScript::parse<NScript::Primary>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	bool local = false;
	switch(token) {
	case parser::value:		if(!skip)	result = _parser.get_value(); _parser.next(); break;
	/*case parser::my:
		if(_parser.next() != parser::name)	throw nscript_error::syntax_error;
		local = true;
		case Parser::name:		ParseVar(result, local, skip);	break;
		case Parser::iffunc:	_parser.Next();Parse(Assignment, result, skip);ParseIf(Assignment, result, skip);break;
		case Parser::forloop:	ParseForLoop(result, skip);break;
		case Parser::idiv:		ParseFunc(result, skip); break;
		case Parser::func:		ParseFunc(result, skip);break;
		case Parser::object:	ParseObj(result, skip);break;*/
	case parser::lpar:
	case parser::lsquare:
		_parser.next();
		if(!skip)	result = 0;
		parse<Statement>(result, skip);
		_parser.check_pair(token);
		break;
	case parser::lcurly:
		_parser.next();
		_context.push();
		parse<Script>(result, skip);
		_context.pop();
		_parser.check_pair(token);
		break;
	case parser::end:		throw nscript_error::unexpected_eof;
	}
}

// Parser

parser::Keywords parser::_keywords = {
	{ "for",	parser::forloop },
	{ "if",		parser::iffunc },
	{ "else",	parser::ifelse },
	{ "sub",	parser::func },
	{ "fn",		parser::func },
	{ "object",	parser::object },
	{ "new",	parser::newobj },
	{ "my",		parser::my },
};

parser::parser() {}

parser::token parser::next()
{
	_lastpos = _pos;
	char_t c, cc;
	while(isspace(c = read()));
	switch(c)	{
		case '\0':	_token = end;break;
		case '+':	cc = peek(); _token = (cc == '+' ? read(), unaryplus  : cc == '=' ? read(), plusset  : plus); break;
		case '-':	cc = peek(); _token = (cc == '-' ? read(), unaryminus : cc == '=' ? read(), minusset : minus); break;
		case '*':	_token = peek() == '=' ? read(), mulset  : multiply;break;
		case '/':	_token = peek() == '=' ? read(), divset  : divide;break;
		case '\\':	_token = peek() == '=' ? read(), idivset : idiv;break;
		case '%':	_token = mod;break;
		case '^':	_token = pwr;break;
		case '~':	_token = not;break;
		case ';':	_token = stmt;while(peek() == c)	read();break;
		case ',':	_token = comma;break;
		case '.':	_token = dot; read_name(read());_value = _name.c_str();break;
		case '<':	_token = peek() == '=' ? read(), le   : lt;break;
		case '>':	_token = peek() == '=' ? read(), ge   : gt;break;
		case '=':	_token = peek() == '=' ? read(), equ  : peek() == '>' ? read(), func : assign;break;
		case '!':	_token = peek() == '=' ? read(), nequ : lnot;break;
		case '&':	_token = peek() == '&' ? read(), land : and;break;
		case '|':	_token = peek() == '|' ? read(), lor  : or;break;
		case '(':	_token = lpar;break;
		case ')':	_token = rpar;break;
		case '{':	_token = lcurly;break;
		case '}':	_token = rcurly;break;
		case '[':	_token = lsquare;break;
		case ']':	_token = rsquare;break;
		case '#':	_token = value; read_date(c); break;
		case '`':	_token = apo; break;
		case 0xB7:	_token = mdot; break;
		case '\"':
		case '\'':	_token = value; read_string(c);break;
		case '?':	_token = ifop;break;
		case ':':	_token = peek() == '=' ? read(), setvar : colon;break;
		default:
			if(isdigit(c))	{
				read_number(c);
			}	else	{
				read_name(c);
				auto p = _keywords.find(_name);
				_token = p == _keywords.end() ? name : _token = p->second;
			}
			break;
	}
	return _token;
}

// Parse integer/double/hexadecimal value from input stream
void parser::read_number(char_t c)
{
	enum number_stage {nsint, nsdot, nsexp, nspwr, nshex} stage = nsint;
	int base = 10, m = c - '0';
	int e1 = 0, e2 = 0, esign = 1;
	bool overflow = false;

	if(c == '0' && tolower(peek()) == 'x')	{stage = nshex; base = 16; read();}

	while(c = read())	{	
		if(isdigit(c))	{
			char_t v = c - '0';
			if(stage == nsint || stage == nshex) {
				if(m > (INT_MAX - v) / base)	throw std::errc::value_too_large;
				m = m * base + v;
			}
			else if(stage == nsexp)		stage = nspwr;
			else if(stage == nsdot && !overflow)	{
				if(m > (INT_MAX - v) / base)	overflow = true;
				else							m = m * base + v, e1--;
			}
			if(stage == nspwr)		e2 = e2 * 10  + v;
		}	else if(isxdigit(c) && stage == nshex)		{
			char_t v = 10 + (toupper(c) - 'A');
			if(m > (INT_MAX - v) / base)	throw std::errc::value_too_large;
			m = m * base + v;
		}	else if(c == '.')		{
			if(stage > nsint)	break;
			stage = nsdot;
		}	else if(c == 'e' || c == 'E' || c == 'd' || c == 'D')		{
			if(stage > nsexp)	break;
			stage = nsexp;
		}	else if(c == '+' || c == '-')		{
			if(stage == nsexp)	esign = c == '-' ? -1 : 1;	else break;
			stage = nspwr;
		}	else	break;
	};
	back();
	if(stage == nsexp)	throw nscript_error::syntax_error;
	if(stage == nsint || stage == nshex)	_value = m;							// integer
	else									_value = m * pow(10., e1+esign*e2);	// floating-point
	_token = parser::value;
}

// Parse date from input stream
void parser::read_date(char_t quote) {
}

// Parse quoted string from input stream
void parser::read_string(char_t quote)	{
	string_t temp;
	for(;;temp += read())	{
		state endpos = _content.find(quote, _pos);
		if(endpos == string_t::npos)	throw nscript_error::missing_character;
		temp += _content.substr(_pos, endpos-_pos).c_str();
		_pos = endpos+1;
		if(peek() != quote)	break;
	}
	_value = temp.c_str();
}

// Parse object name from input stream
void parser::read_name(char_t c)	
{
	if(!isalpha(c) && c != '@' && c != '_')		throw nscript_error::syntax_error;
	_name = c;
	while(isalnum(c = read()) || c == '_')	_name += c;
	back();
}

void parser::check_pair(parser::token token)
{
	if(token == lpar && _token != rpar)			throw nscript_error::missing_character;
	if(token == lsquare && _token != rsquare)	throw nscript_error::missing_character;
	if(token == lcurly && _token != rcurly)		throw nscript_error::missing_character;
	if(token == lpar || token == lsquare || token == lcurly)	next();
}

}