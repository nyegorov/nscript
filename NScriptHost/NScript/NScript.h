// NScript - lightweight, single-pass parser for executing c-style expressions. 
// Supports integer, double, string and date types. Has limited set of flow 
// control statements (if/then/else, loops). Fully extensible by user-defined 
// objects with IObject interface.

#pragma once

#include <comdef.h>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace nscript	{

#ifdef _UNICODE
#define tchar		wchar_t
#define tstring		std::wstring
#else
#define tchar		char
#define tstring		std::string
#endif

#define FACILITY_NSCRIPT		0x452
#define E_NS_RUNTIMEERROR		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 0)
#define E_NS_UNEXPECTEDEOF		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 1)
#define E_NS_MISSINGCHARACTER	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 2)
#define E_NS_UNKNOWNVARIABLE	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 3)
#define E_NS_TOOMANYITERATIONS	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 4)
#define E_NS_SYNTAXERROR		MAKE_HRESULT(SEVERITY_ERROR, FACILITY_NSCRIPT, 5)

// Interface for extension objects
struct __declspec(uuid("{BF4F1403-B407-42aa-B642-F57B33FED8A1}")) IObject : public IUnknown	{
	STDMETHOD(New)(variant_t& result) PURE;								// result = new <object>
	STDMETHOD(Get)(variant_t& result) PURE;								// result = <object>
	STDMETHOD(Set)(const variant_t& value) PURE;						// <object> = value
	STDMETHOD(Call)(const variant_t& params, variant_t& result) PURE;	// result = <object>(params)
	STDMETHOD(Item)(const variant_t& item, variant_t& result) PURE;		// result = <object>.item
	STDMETHOD(Index)(const variant_t& index, variant_t& result) PURE;	// result = <object>[index]
};
#define IID_IObject	__uuidof(IObject)
_COM_SMARTPTR_TYPEDEF(IObject, IID_IObject);

// Interface for objects, accessible by reference (L-values)
struct __declspec(uuid("{AD60A84C-990F-4341-9477-F8830C8D3415}")) IValue : public IUnknown	{
	virtual STDMETHODIMP GetRef(variant_t** ppvar) = 0;
};
#define IID_IValue	__uuidof(IValue)
_COM_SMARTPTR_TYPEDEF(IValue, IID_IValue);

struct lessi	{bool operator () (const tstring& s1, const tstring &s2) const {return lstrcmpi(s1.c_str(), s2.c_str()) < 0;}};
using args_list = std::vector<tstring>;

// Container for storing named objects and variables
class Context	
{
public:
	typedef std::set<tstring, lessi>			VarNames;
	Context(const Context *base, const VarNames *vars = NULL);
	void Push()		{_locals.push_back(vars_t());}
	void Pop()		{_locals.pop_back();}
	variant_t& Get(const tstring& name, bool local = false);
	bool Get(const tstring& name, variant_t& result) const;
	void Set(const tstring& name, const variant_t& value)		{_locals.front()[name] = value;}
private:
	typedef std::map<tstring, variant_t, lessi>	vars_t;
	static vars_t		_globals;
	std::vector<vars_t>	_locals;
};

// Parser of input stream to a list of tokens
class Parser	{
public:
	typedef size_t	State;
	enum Token	{end,mod,assign,ge,gt,le,lt,nequ,name,value,land,lor,lnot,stmt,err,dot,newobj,minus,lpar,rpar,lcurly,rcurly,equ,plus,lsquare,rsquare,multiply,divide,idiv,and,or,not,pwr,comma,unaryplus,unaryminus,forloop,ifop,iffunc,ifelse,func,object,plusset, minusset, mulset, divset, idivset, setvar,my,colon,apo,mdot};

	Parser();
	void Init(const tchar* expr){if(expr)	_content = expr;SetState(0);}
	Token GetToken()			{return _token;}
	const variant_t& GetValue()	{return _value;}
	const tstring& GetName()	{return _name;}
	State GetState()			{return _lastpos;}
	void SetState(State state)	{_pos = state;_token=end;Next();}
	tstring GetContent(State begin, State end)	{return _content.substr(begin, end-begin);}
	void CheckPairedToken(Token token);
	Token Next();
private:
	typedef std::map<tstring, Token, lessi> Keywords;
	static Keywords	_keywords;
	tchar			_decpt = std::use_facet<std::numpunct<tchar>>(std::locale()).decimal_point();
	Token			_token;
	tstring			_content;
	State			_pos = 0;
	State			_lastpos = 0;
	variant_t		_value;
	tstring			_name;

	tchar Peek()			{return _pos >= _content.length() ? 0 : _content[_pos];}
	tchar Read()			{tchar c = Peek();_pos++;return c;}
	void Back()				{_pos--;}
	void ReadString(tchar quote);
	void ReadNumber(tchar c);
	void ReadName(tchar c);
};

// Main class for executing scripts
class NScript
{
public:
	NScript(const tchar* script = NULL, const Context *pcontext = NULL) : _context(pcontext)	{_parser.Init(script);}
	~NScript(void)						{};
	HRESULT Exec(const tchar* script, variant_t& result);
	HRESULT AddObject(const tchar* name, const variant_t& object)	{return _context.Set(name, object), S_OK;}

protected:
	friend class Class; 
	friend class Function;
	enum Precedence	{Script = 0,Statement,Assignment,Conditional,Logical,Binary,Equality,Relation,Addition,Multiplication,Power,Unary,Functional,Primary,Term};

	void Parse(Precedence level, variant_t& result, bool skip);
	void Parse(Precedence level, Parser::State state, variant_t& result);
	void ParseIf(Precedence level, variant_t& result, bool skip);
	void ParseForLoop(variant_t& result, bool skip);
	void ParseArgList(args_list& args, bool forceArgs);
	void ParseFunc(variant_t& args, bool skip);
	void ParseObj(variant_t& result, bool skip);
	void ParseVar(variant_t& result, bool local, bool skip);

	Parser				_parser;
	Context				_context;
	Context::VarNames	_varnames;

	typedef void OpFunc(variant_t& op1, variant_t& op2, variant_t& result);
	static struct OpInfo {Parser::Token token; OpFunc* op;}	_operators[Term][10];
};

// Generic implementation of IObject interface
class Object : public IObject	{
public:
	Object() : _counter(0)		{};
	STDMETHODIMP_(ULONG) AddRef()	{return InterlockedIncrement(&_counter);}
	STDMETHODIMP_(ULONG) Release()	{return InterlockedDecrement(&_counter) > 0 ? _counter : (delete this, 0);}
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)	{
		if(!ppvObj) return E_POINTER;
		if (iid == IID_IUnknown)	{*ppvObj = static_cast<IUnknown*>(this); AddRef(); return S_OK;}
		if (iid == IID_IObject)		{*ppvObj =  static_cast<IObject*>(this); AddRef(); return S_OK;}
		return *ppvObj = NULL, E_NOINTERFACE;
	}
	STDMETHODIMP New(variant_t& result)								{return E_NOTIMPL;}
	STDMETHODIMP Get(variant_t& result)								{
		if(V_VT(&result) != VT_UNKNOWN || V_UNKNOWN(&result) != (IUnknown*)this)	result = this; 
		return S_OK;
	}
	STDMETHODIMP Set(const variant_t& value)						{return E_NOTIMPL;}
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{return E_NOTIMPL;}
	STDMETHODIMP Item(const variant_t& item, variant_t& result)		{return E_NOTIMPL;}
	STDMETHODIMP Index(const variant_t& index, variant_t& result)	{return E_NOTIMPL;}
protected:
	virtual ~Object()	{};
	long		_counter;
};

// Wrapper for SafeArray* set of API functions
class SafeArray {
public:
	SafeArray(variant_t& sa) :_sa(sa), _count(0)	{};
	~SafeArray();
	void Put(long index, const variant_t& value, bool forceArray = false);
	void Add(const variant_t& value)				{Put(Count(), value);}
	void Redim(long size);
	void Append(const variant_t& value);
	void Get(long index, variant_t& value) const;
	void Remove(long index);
	long Count() const;
	variant_t* GetData();
	variant_t& operator [](long index);

protected:
	bool IsArray() const { return V_ISARRAY(&_sa) != 0; }
	bool IsEmpty() const { return V_VT(&_sa) == VT_EMPTY; }

	variant_t&	_sa;
	int			_count;
};

// Class that represents script variables
class Variable	: public Object, public IValue {
public:
	Variable() : _value()											{V_VT(&_value) = VT_NULL;}
	STDMETHODIMP_(ULONG) AddRef()									{return Object::AddRef();}
	STDMETHODIMP_(ULONG) Release()									{return Object::Release();}
	STDMETHODIMP QueryInterface(REFIID iid, void ** ppvObj)			{
		if (ppvObj && iid == IID_IValue)	{*ppvObj =  static_cast<IValue*>(this); AddRef(); return S_OK;}	
		else return Object::QueryInterface(iid, ppvObj);
	}
	STDMETHODIMP Get(variant_t& result)								{result = *GetPtr(); return V_VT(&_value) == VT_NULL ? DISP_E_UNKNOWNNAME : S_OK;}
	STDMETHODIMP Set(const variant_t& value)						{*GetPtr() = value; return S_OK;}
	STDMETHODIMP New(variant_t& result)								{return IObjectPtr(*GetPtr())->New(result);}
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{return IObjectPtr(*GetPtr())->Call(params, result);}
	STDMETHODIMP Item(const variant_t& item, variant_t& result)		{return IObjectPtr(*GetPtr())->Item(item, result);}
	STDMETHODIMP Index(const variant_t& index, variant_t& result);
	STDMETHODIMP GetRef(variant_t** ppvar)							{*ppvar = &_value;return S_OK;}
protected:
	variant_t* GetPtr()												{variant_t *pv;GetRef(&pv);return pv;}
	~Variable()			{}
	variant_t			_value;
};

// Proxy object for array index operator (i.e., a[i])
class Indexer : public Variable	{
public:
	Indexer(IValuePtr value, const variant_t& index) : _index(index), _value(value)	{};
	STDMETHODIMP GetRef(variant_t** ppvar)							{_value->GetRef(ppvar);*ppvar = &(SafeArray(**ppvar)[_index]);return S_OK;}
	STDMETHODIMP Get(variant_t& result);
protected:
	~Indexer()	{}
	IValuePtr	_value;
	long		_index;
};

// User-defined functions
class Function	: public Object {
public:
	Function(const args_list& args, const tchar* body, const Context *pcontext = NULL) : _args(args), _body(body), _context(pcontext)	{}
	STDMETHODIMP Call(const variant_t& params, variant_t& result);
protected:
	~Function()			{}
	const args_list		_args;
	const tstring		_body;
	const Context		_context;
};

// User-defined classes
class Class	: public Object {
public:
	Class(const args_list& args, const tchar* body, const Context *pcontext = NULL) : _args(args), _body(body), _context(pcontext)	{}
	STDMETHODIMP New(variant_t& result);
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{_params = params; result = this; return S_OK;}

	class Instance	: public Object {
	public:
		Instance(const tchar* body, const Context *pcontext) : _script(body, pcontext)	{}
		STDMETHODIMP Init(const args_list& args, const variant_t& params);
		STDMETHODIMP Item(const variant_t& item, variant_t& result);
	protected:
		~Instance()			{}
		NScript				_script;
	};

protected:
	~Class()			{}
	const tstring		_body;
	variant_t			_params;
	const Context		_context;
	const args_list		_args;
};

// Built-in functions
class BuiltinFunction : public Object	{
public:
	typedef void FN(int n, const variant_t v[], variant_t& result);
	BuiltinFunction(int count, FN *pfunc) : _count(count), _pfunc(pfunc)	{}
	STDMETHODIMP Call(const variant_t& params, variant_t& result)	{
		SafeArray a(const_cast<variant_t&>(params));
		if(_count >= 0 && _count != a.Count())	return DISP_E_BADPARAMCOUNT;
		(*_pfunc)(a.Count(), a.GetData(), result); 
		return S_OK;
	}
protected:
	~BuiltinFunction()	{}
	const int			_count;
	FN*					_pfunc;
};

class Composer : public Object {
public:
	Composer(const variant_t& left, const variant_t& right) : _left(left), _right(right) {}
	STDMETHODIMP Call(const variant_t& params, variant_t& result) {
		variant_t tmp;
		HRESULT hr = _right->Call(params, tmp);
		return SUCCEEDED(hr) ? _left->Call(tmp, result) : hr;
	}
protected:
	IObjectPtr		_left;
	IObjectPtr		_right;
};

}
