#include "stdafx.h"
#include <locale>
#include <limits>
#include <algorithm>
#include <functional>
#include <math.h>
#include <time.h>
#include <tchar.h>
#include <assert.h>
#include "NScript.h"

#undef min
#undef max

namespace nscript	{

// Globals

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

// throw exception when hr fails
void Check(HRESULT hr, const void* param = TEXT(""), Parser* parser = NULL)	{
	if(FAILED(hr))	{
		if(HRESULT_FACILITY(hr) == FACILITY_NSCRIPT)	SetError(hr, param, parser);
		throw _com_error(hr);
	}
}

// process list of argument
HRESULT ProcessArgsList(const Parser::ArgList& args, const variant_t& params, NScript& script)	{
	SafeArray a(const_cast<variant_t&>(params));
	if(args.size() == 0)	{
		script.AddObject(TEXT("@"),params);
	}	else	{
		if(args.size() != a.Count())	return DISP_E_BADPARAMCOUNT;
		const variant_t *pargs = a.GetData();
		for(int i = a.Count()-1; i>=0; i--)	script.AddObject(args[i].c_str(),pargs[i]);
	}
	return S_OK;
}

// 'dereference' object. If v holds an ext. object, replace it with the value of object
variant_t& operator *(variant_t& v)	{
	if(V_VT(&v) == VT_UNKNOWN)	{
		IObjectPtr pobj = v;
		if(pobj)	Check(pobj->Get(v));
	}
	return v;
}

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

STDMETHODIMP Class::Instance::Init(const ArgList& args, const variant_t& params)	{
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

// Operators

void OpAdd(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarAdd(&*op1, &*op2, &result));}
void OpSub(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarSub(&*op1, &*op2, &result));}
void OpMul(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarMul(&*op1, &*op2, &result));}
void OpDiv(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarDiv(&*op1, &*op2, &result));}
void OpIDiv(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarIdiv(&*op1, &*op2, &result));}
void OpMod(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarMod(&*op1, &*op2, &result));}
void OpPow(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarPow(&*op1, &*op2, &result));}
void OpAnd(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarAnd(&*op1, &*op2, &result));}
void OpOr(variant_t& op1, variant_t& op2, variant_t& result)	{Check(VarOr(&*op1, &*op2, &result));}
void OpGT(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)==VARCMP_GT;}
void OpGE(variant_t& op1, variant_t& op2, variant_t& result)	{result = VarCmp(&*op1, &*op2, LOCALE_USER_DEFAULT)>=VARCMP_GT;}
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
void OpCall(variant_t& op1, variant_t& op2, variant_t& result)	{Check(IObjectPtr(op1)->Call(*op2, result));}
void OpAssign(variant_t& op1, variant_t& op2, variant_t& result){Check(IObjectPtr(op1)->Set(*op2));result = op1;}
void OpItem(variant_t& op1, variant_t& op2, variant_t& result)	{Check(IObjectPtr(op1)->Item(*op2, result));}
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
void FnRemove(int n, const variant_t v[], variant_t& result)	{
	variant_t vt;
	SafeArray a(result=v[0]);
	for(long i=v[1];i<a.Count()-1;i++)	{
		a.Get(i+1,vt);
		a.Put(i, vt);
	}
	a.Redim(a.Count()-1);
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

// Context
struct copy_var {
	const Context	*_from;
	Context			*_to;
	copy_var(const Context *from ,Context *to) : _from(from), _to(to)	{};
	void operator() (const tstring& name) {variant_t v; if(_from->Get(name, v))	_to->Set(name, v);}
};

Context::vars_t	Context::_globals;
Context::Context(const Context *base, const VarNames *vars) : _locals(1)
{
	if(base)	
		if(vars)	std::for_each(vars->begin(), vars->end(), copy_var(base, this));
		else		_locals.assign(base->_locals.begin(), base->_locals.end());
		//_locals[0] = base->_locals[0];
	if(_globals.empty())	{
		// Constants
		typedef vars_t::value_type pair;
		_globals.insert(pair(TEXT("empty"),		variant_t()));
		_globals.insert(pair(TEXT("true"),		true));
		_globals.insert(pair(TEXT("false"),		false));
		_globals.insert(pair(TEXT("optional"),	variant_t(DISP_E_PARAMNOTFOUND, VT_ERROR)));
		// Conversion
		_globals.insert(pair(TEXT("int"),		new BuiltinFunction( 1, FnCvt<VT_I4, LOCALE_INVARIANT>  )));
		_globals.insert(pair(TEXT("dbl"),		new BuiltinFunction( 1, FnCvt<VT_R8, LOCALE_INVARIANT>  )));
		_globals.insert(pair(TEXT("str"),		new BuiltinFunction( 1, FnCvt<VT_BSTR, LOCALE_USER_DEFAULT>)));
		_globals.insert(pair(TEXT("date"),		new BuiltinFunction( 1, FnCvt<VT_DATE, LOCALE_USER_DEFAULT>)));
		// Date
		_globals.insert(pair(TEXT("now"),		new BuiltinFunction( 0, FnNow)));
		_globals.insert(pair(TEXT("day"),		new BuiltinFunction( 1, FnDay)));
		_globals.insert(pair(TEXT("month"),		new BuiltinFunction( 1, FnMonth)));
		_globals.insert(pair(TEXT("year"),		new BuiltinFunction( 1, FnYear)));
		_globals.insert(pair(TEXT("hour"),		new BuiltinFunction( 1, FnHour)));
		_globals.insert(pair(TEXT("minute"),	new BuiltinFunction( 1, FnMinute)));
		_globals.insert(pair(TEXT("second"),	new BuiltinFunction( 1, FnSecond)));
		_globals.insert(pair(TEXT("dayofweek"),	new BuiltinFunction( 1, FnDayOfWeek)));
		_globals.insert(pair(TEXT("dayofyear"),	new BuiltinFunction( 1, FnDayOfYear)));
		// Math
		_globals.insert(pair(TEXT("pi"),		new BuiltinFunction( 0, FnPi)));
		_globals.insert(pair(TEXT("rnd"),		new BuiltinFunction( 0, FnRnd)));
		_globals.insert(pair(TEXT("sin"),		new BuiltinFunction( 1, FnX<double,  sin>)));
		_globals.insert(pair(TEXT("cos"),		new BuiltinFunction( 1, FnX<double,  cos>)));
		_globals.insert(pair(TEXT("tan"),		new BuiltinFunction( 1, FnX<double,  tan>)));
		_globals.insert(pair(TEXT("atan"),		new BuiltinFunction( 1, FnX<double, atan>)));
		_globals.insert(pair(TEXT("abs"),		new BuiltinFunction( 1, FnX<double, fabs>)));
		_globals.insert(pair(TEXT("exp"),		new BuiltinFunction( 1, FnX<double,  exp>)));
		_globals.insert(pair(TEXT("log"),		new BuiltinFunction( 1, FnX<double,  log>)));
		_globals.insert(pair(TEXT("sqr"),		new BuiltinFunction( 1, FnX<double, sqrt>)));
		_globals.insert(pair(TEXT("sqrt"),		new BuiltinFunction( 1, FnX<double, sqrt>)));
		_globals.insert(pair(TEXT("atan2"),		new BuiltinFunction( 2, FnAtan2)));
		_globals.insert(pair(TEXT("sgn"),		new BuiltinFunction( 1, FnSgn)));
		_globals.insert(pair(TEXT("fract"),		new BuiltinFunction( 1, FnFract)));
		// String
		_globals.insert(pair(TEXT("chr"),		new BuiltinFunction( 1, FnChr)));
		_globals.insert(pair(TEXT("asc"),		new BuiltinFunction( 1, FnAsc)));
		_globals.insert(pair(TEXT("len"),		new BuiltinFunction( 1, FnLen)));
		_globals.insert(pair(TEXT("left"),		new BuiltinFunction( 2, FnLeft)));
		_globals.insert(pair(TEXT("right"),		new BuiltinFunction( 2, FnRight)));
		_globals.insert(pair(TEXT("mid"),		new BuiltinFunction( 3, FnMid)));
		_globals.insert(pair(TEXT("upper"),		new BuiltinFunction( 1, FnUpper)));
		_globals.insert(pair(TEXT("lower"),		new BuiltinFunction( 1, FnLower)));
		_globals.insert(pair(TEXT("string"),	new BuiltinFunction( 2, FnString)));
		_globals.insert(pair(TEXT("replace"),	new BuiltinFunction( 3, FnReplace)));
		_globals.insert(pair(TEXT("instr"),		new BuiltinFunction( 2, FnInstr)));
		_globals.insert(pair(TEXT("format"),	new BuiltinFunction( 2, FnFormat)));
		_globals.insert(pair(TEXT("hex"),		new BuiltinFunction( 1, FnHex)));
		// Array
		_globals.insert(pair(TEXT("add"),		new BuiltinFunction( 2, FnAdd)));
		_globals.insert(pair(TEXT("remove"),	new BuiltinFunction( 2, FnRemove)));
		// Other
		_globals.insert(pair(TEXT("size"),		new BuiltinFunction(-1, FnSize)));
		_globals.insert(pair(TEXT("rgb"),		new BuiltinFunction( 3, FnRgb)));
		_globals.insert(pair(TEXT("min"),		new BuiltinFunction(-1, FnFind<VARCMP_LT> )));
		_globals.insert(pair(TEXT("max"),		new BuiltinFunction(-1, FnFind<VARCMP_GT> )));
	}
}

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

// Operator precedence
NScript::OpInfo NScript::_operators[Term][10] = {
	{{Parser::stmt,		&OpNull},	{Parser::end, NULL}},
	{{Parser::comma,	&OpNull},	{Parser::end, NULL}},
	{{Parser::assign,	&OpAssign},	{Parser::plusset, &OpXSet<OpAdd>}, {Parser::minusset, &OpXSet<OpSub>}, {Parser::mulset, &OpXSet<OpMul>}, {Parser::divset, &OpXSet<OpDiv>}, {Parser::idivset, &OpXSet<OpIDiv>}, {Parser::end, NULL}},
	{{Parser::ifop,		&OpNull},	{Parser::end, NULL}},
	{{Parser::land,		&OpLAnd},	{Parser::lor,	&OpLOr},	{Parser::end, NULL}},
	{{Parser::and,		&OpAnd},	{Parser::or,	&OpOr},		{Parser::end, NULL}},
	{{Parser::equ,		&OpEqu},	{Parser::nequ,	&OpNeq},	{Parser::end, NULL}},
	{{Parser::gt,		&OpGT},		{Parser::ge,	&OpGE},		{Parser::lt, &OpLT},	{Parser::le, &OpLE},	{Parser::end, NULL}},
	{{Parser::plus,		&OpAdd},	{Parser::minus,	&OpSub},	{Parser::end, NULL}},
	{{Parser::multiply,	&OpMul},	{Parser::divide,&OpDiv},	{Parser::idiv, &OpIDiv},{Parser::mod, &OpMod},	{Parser::end, NULL}},
	{{Parser::pwr,		&OpPow},	{Parser::end, NULL}},
	{{Parser::unaryplus,&OpPPX},	{Parser::unaryminus,&OpMMX},{Parser::newobj,&OpNew},{Parser::not,	&OpNot},{Parser::lnot, &OpLNot},{Parser::minus, &OpNeg},	{Parser::end, NULL}},
	{{Parser::unaryplus,&OpXPP},	{Parser::unaryminus,&OpXMM},{Parser::lpar, &OpCall},{Parser::dot, &OpItem}, {Parser::lsquare, &OpIndex},{Parser::end, NULL}},
};

HRESULT NScript::Exec(const tchar* script, variant_t& result)
{
	HRESULT hr = S_OK;
	SetErrorInfo(0, NULL);
	_context.Push();
	try	{
		_parser.Init(script);
		result.Clear();
		Parse(Script, result, false);
		if(_parser.GetToken()!=Parser::end)	Check(E_NS_SYNTAXERROR, NULL, &_parser);
		*result;
	}	catch (std::exception& e)	{SetError(hr = E_NS_RUNTIMEERROR, e.what());
	}	catch (_com_error e)		{if(HRESULT_FACILITY(hr = e.Error()) != FACILITY_NSCRIPT)	SetError(hr, e.ErrorMessage(), &_parser);
	}	catch (...)					{hr = E_FAIL;}
	_context.Pop();
	return hr;
}

// Parse "if <cond> <true-part> [else <part>]" statement
void NScript::ParseIf(variant_t& result, bool skip)	{
	bool cond = skip || (bool)result;
	Parse(Assignment, result, !cond || skip);
	if(_parser.GetToken() == Parser::ifelse)	{
		_parser.Next();
		Parse(Assignment, result, cond || skip);
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

// Parse "sub [(<arguments>)] <body>" statement
void NScript::ParseFunc(variant_t& result, bool skip)
{
	Parser::ArgList args = _parser.GetArgs();
	_parser.Next();
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
	Parser::ArgList args = _parser.GetArgs();
	_parser.Next();
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

void NScript::Parse(Precedence level, variant_t& result, bool skip)
{
	// main parse loop
	Parser::Token token = _parser.GetToken();
	if(level == Primary)	{
		// primary expressions
		bool local = false;
		switch(token)	{
			case Parser::value:		if(!skip)	result = _parser.GetValue();_parser.Next();break;
			case Parser::my:	
				if(_parser.Next() != Parser::name)	Check(E_NS_SYNTAXERROR, NULL, &_parser);
				local = true;
			case Parser::name:		ParseVar(result, local, skip);	break;
			case Parser::iffunc:	_parser.Next();Parse(Assignment, result, skip);ParseIf(result, skip);break;
			case Parser::forloop:	ParseForLoop(result, skip);break;
			case Parser::func:		ParseFunc(result, skip);break;
			case Parser::object:	ParseObj(result, skip);break;
			case Parser::lpar:
			case Parser::lsquare:
				_parser.Next();
				result.Clear();
				Parse(Statement, result, skip);
				_parser.CheckPairedToken(token);
				break;
			case Parser::lcurly:
				_parser.Next();
				_context.Push();
				Parse(Script, result, skip);
//				*result;
				_context.Pop();
				_parser.CheckPairedToken(token);
				break;
			case Parser::end:		Check(E_NS_UNEXPECTEDEOF);break;
			default:				break;
		}
	}	else if (level == Statement)	{
		// comma operator, non-associative
		Parse((Precedence)(level+1), result, skip);
		if(_parser.GetToken() == Parser::comma)	{
			variant_t v = result;
			SafeArray a(result);
			result.Clear();
			a.Put(0, *v, true);
			do	{
				_parser.Next();
				Parse((Precedence)(level+1), v, skip);
				a.Add(*v);
			}	while(_parser.GetToken() == Parser::comma);
		}
	}	else	{
		// all other
		bool noop = true;

		// parse left-hand operand (for binary operators)
		if(level != Unary)	Parse((Precedence)(level+1), result, skip);
again:
		if(_parser.GetToken() == Parser::end)	return;
		for(OpInfo *pinfo = _operators[level];pinfo->op;pinfo++)	{
			token = pinfo->token;
			if(token == _parser.GetToken())	{
				if(token != Parser::lpar && token != Parser::lsquare && token != Parser::dot)	_parser.Next();
				if(token == Parser::ifop)	{		// ternary "a?b:c" operator
					ParseIf(result, skip);
					goto again;
				}

				variant_t left(result), right(0L, VT_ERROR);
				if(!skip && level != Script)	result.Clear();

				// parse right-hand operand
				if(token == Parser::dot)	{right = _parser.GetValue(); _parser.Next();}		// special case for '.' operator
				else if(level == Assignment || level == Unary)	Parse(level, right, skip);		// right-associative operators
				else if(token != Parser::unaryplus && token != Parser::unaryminus && !(level == Script && _parser.GetToken() == Parser::rcurly))	
					Parse((Precedence)(level+1), level == Script ? result : right, skip);		// left-associative operators

				// perform operator's action
				if(!skip)	(*pinfo->op)(left, right, result);
				noop = false;

				if(level == Unary)	break;
				goto again;
			}
		}
		// for unary operators, return right-hand expression if no operators were found
		if(level == Unary && noop)	Parse((Precedence)(level+1), result, skip);
	}
}

// Parser

Parser::Keywords Parser::_keywords;
tchar Parser::_decpt;

Parser::Parser() {
	if(_keywords.empty())	{
		_keywords[TEXT("for")]		= Parser::forloop;
		_keywords[TEXT("if")]		= Parser::iffunc;
		_keywords[TEXT("else")]		= Parser::ifelse;
		_keywords[TEXT("sub")]		= Parser::func;
		_keywords[TEXT("object")]	= Parser::object;
		_keywords[TEXT("new")]		= Parser::newobj;
		_keywords[TEXT("my")]		= Parser::my;
		_decpt = std::use_facet<std::numpunct<tchar> >(std::locale()).decimal_point();
		srand((unsigned)time(NULL));
	}
	_temp.reserve(32);
	_name.reserve(32);
	_pos = _lastpos = 0;
}

Parser::Token Parser::Next()
{
	_lastpos = _pos;
	tchar c, cc;
	while(isspace(c = Read()));
	switch(c)	{
		case '\0':	_token = end;break;
		case '+':	cc = Peek(); _token = (cc == '+' ? Read(), unaryplus  : cc == '=' ? Read(), plusset  : plus); break;
		case '-':	cc = Peek(); _token = (cc == '-' ? Read(), unaryminus : cc == '=' ? Read(), minusset : minus); break;
		case '*':	_token = Peek() == '=' ? Read(), mulset  : multiply;break;
		case '/':	_token = Peek() == '=' ? Read(), divset  : divide;break;
		case '\\':	_token = Peek() == '=' ? Read(), idivset : idiv;break;
		case '%':	_token = mod;break;
		case '^':	_token = pwr;break;
		case '~':	_token = not;break;
		case ';':	_token = stmt;while(Peek() == c)	Read();break;
		case ',':	_token = comma;break;
		case '.':	_token = dot;ReadName(Read());_value = _name.c_str();break;
		case '<':	_token = Peek() == '=' ? Read(), le   : lt;break;
		case '>':	_token = Peek() == '=' ? Read(), ge   : gt;break;
		case '=':	_token = Peek() == '=' ? Read(), equ  : assign;break;
		case '!':	_token = Peek() == '=' ? Read(), nequ : lnot;break;
		case '&':	_token = Peek() == '&' ? Read(), land : and;break;
		case '|':	_token = Peek() == '|' ? Read(), lor  : or;break;
		case '(':	_token = lpar;break;
		case ')':	_token = rpar;break;
		case '{':	_token = lcurly;break;
		case '}':	_token = rcurly;break;
		case '[':	_token = lsquare;break;
		case ']':	_token = rsquare;break;
		case '#':	_token = value;ReadString(c);_value.ChangeType(VT_DATE);break;
		case '\"': 
		case '\'':	_token = value;ReadString(c);break;
		case '?':	_token = ifop;break;
		case ':':	_token = Peek() == '=' ? Read(), setvar : ifelse;break;
		default:
			if(isdigit(c))	{
				ReadNumber(c);
			}	else	{
				ReadName(c);
				Keywords::const_iterator p = _keywords.find(_name);
				if(p == _keywords.end())	{
					_token = name;
				}	else	{
					_token = p->second;
					if(_token == func || _token == object)	ReadArgList();
				}
			}
			break;
	}
	return _token;
}

// Parse integer/double/hexadecimal value from input stream
void Parser::ReadNumber(tchar c)
{
	enum NumberStage {nsint,nsdot,nsexp,nspwr,nshex} stage = nsint;
	int64_t base = 10, m = c - '0';
	int e1 = 0, e2 = 0, esign = 1;
	bool overflow = false;

	if(c == '0' && tolower(Peek()) == 'x')	{stage = nshex; base = 16; Read();}

	while(c = Read())	{	
		if(isdigit(c))	{
			tchar v = c - '0';
			if(stage == nsint || stage == nshex)
				m = m > (LLONG_MAX - v) / base ? Check(DISP_E_OVERFLOW, NULL, this), 0 : m * base + v;
			else if(stage == nsexp)		stage = nspwr;
			else if(stage == nsdot && !overflow)	{
				if(m > (LLONG_MAX - v) / base)	overflow = true;
				else							m = m * base + v, e1--;
			}
			if(stage == nspwr)		e2 = e2 * 10  + v;
		}	else if(isxdigit(c) && stage == nshex)		{
			tchar v = 10 + (toupper(c) - 'A');
			m = m > (LLONG_MAX - v) / base ? Check(DISP_E_OVERFLOW, NULL, this), 0 : m * base + v;
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
	Back();
	if(stage == nsexp)	Check(E_NS_SYNTAXERROR);
	if(stage == nsint || stage == nshex)	{
		if(m < LONG_MAX)	_value = (long)m;					// integers
		else				_value = m;							// long integers (64-bit)
	}	else				_value = m * pow(10., e1+esign*e2);	// floating-point
	_token = Parser::value;
}

// Parse quoted string from input stream
void Parser::ReadString(tchar quote)	{
	_temp.clear();
	for(;;_temp += Read())	{
		State endpos = _content.find(quote, _pos);
		if(endpos == tstring::npos)	Check(E_NS_MISSINGCHARACTER, (void *)quote, this);
		_temp += _content.substr(_pos, endpos-_pos).c_str();
		_pos = endpos+1;
		if(Peek() != quote)	break;
	}
	_value = _temp.c_str();
}

// Parse comma-separated arguments list from input stream
void Parser::ReadArgList()	{
	_args.clear();
	_temp.clear();
	tchar c;
	while(isspace(c = Read()));
	if(c != '(')	{Back(); return;}		// function without parameters

	while(c = Read()) {
		if(isspace(c))	{
		}	else if(c == ',')	{
			_args.push_back(_temp);
			_temp.clear();
		}	else if(c == ')')	{
			if(!_temp.empty()) _args.push_back(_temp);
			break;
		}	else if(!isalnum(c) && c!= '_'){
			Check(E_NS_SYNTAXERROR, NULL, this);
		}	else	_temp += c;
	}
}

// Parse object name from input stream
void Parser::ReadName(tchar c)	
{
	if(!isalpha(c) && c != '@' && c != '_')	Check(E_NS_SYNTAXERROR, NULL, this);
	_name = c;
	while(isalnum(c = Read()) || c == '_')	_name += c;
	Back();
}

void Parser::CheckPairedToken(Parser::Token token)
{
	if(token == lpar && _token != rpar)			Check(E_NS_MISSINGCHARACTER, (void*)')', this);
	if(token == lsquare && _token != rsquare)	Check(E_NS_MISSINGCHARACTER, (void*)']', this); 
	if(token == lcurly && _token != rcurly)		Check(E_NS_MISSINGCHARACTER, (void*)'}', this);
	if(token == lpar || token == lsquare || token == lcurly)	Next();
}

}