// NScript - lightweight, single-pass parser for executing c-style expressions. 
// Supports integer, double, string and date types. Has limited set of flow 
// control statements (if/then/else, loops). Fully extensible by user-defined 
// objects with i_object interface.

#pragma once

#include <chrono>
#include <optional>
#include <variant>
#include <string>
#include <string_view>
#include <system_error>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <vector>
#include <map>
#include <set>

namespace nscript3	{

enum class nscript_error {runtime_error = 1001, unexpected_eof, missing_character, unknown_var, missing_lval, syntax_error, bad_param_count, type_mismatch };

class nscript_category_impl : public std::error_category
{
public:
	virtual const char * name() const noexcept { return "nscript"; }
	virtual std::string message(int ev) const {
		switch(static_cast<nscript_error>(ev))	{
		case nscript_error::runtime_error:		return "runtime error";
		case nscript_error::unexpected_eof:		return "unexpected end";
		case nscript_error::missing_character:	return "missing character";
		case nscript_error::missing_lval:		return "missing lvalue";
		case nscript_error::unknown_var:		return "unknown variable";
		case nscript_error::syntax_error:		return "syntax error";
		case nscript_error::bad_param_count:	return "bad parameters count";
		case nscript_error::type_mismatch:		return "type mismatch";
		default:								return "unknown error";
		}
	};
};

const std::error_category& nscript_category();
std::error_code make_error_code(nscript_error e);
std::error_condition make_error_condition(nscript_error e);

struct i_object;
using std::string_view;
using string_t = std::string;
using date_t = std::chrono::system_clock::time_point;
using object_ptr = std::shared_ptr<i_object>;
using value_t = std::variant<std::monostate, int, double, string_t, date_t, object_ptr>;
using params_t = std::vector<value_t>;

std::string to_string(value_t v);
int to_int(value_t v);
double to_double(value_t v);
date_t to_date(value_t v);

// Interface for extension objects
struct i_object {
	virtual value_t create() const = 0;					// return new <object>
	virtual value_t get()  = 0;							// return <object>
	virtual void set(value_t value) = 0;				// <object> = value
	virtual value_t call(value_t params) = 0;			// return <object>(params)
	virtual value_t item(value_t item) = 0;				// return <object>.item
	virtual value_t index(value_t index) = 0;			// return <object>[index]
	virtual string_t print() const = 0;
};

using args_list = std::vector<string_t>;

// Container for storing named objects and variables
class context	
{
public:
	typedef std::unordered_set<string_t>			var_names;
	context(const context *base, const var_names *vars = NULL);
	void push()		{_locals.push_back(vars_t());}
	void pop()		{_locals.pop_back();}
	value_t& get(string_t name, bool local = false);
	std::optional<value_t> get(string_t name) const;
	void set(string_t name, value_t value)		{_locals.front()[name] = value;}
private:
	typedef std::unordered_map<string_t, value_t>	vars_t;
	static vars_t		_globals;
	std::vector<vars_t>	_locals;
};

// Parser of input stream to a list of tokens
class parser	{
public:
	using char_t = string_t::value_type;
	using state = size_t;
	enum token	{end,mod,assign,ge,gt,le,lt,nequ,name,value,land,lor,lnot,stmt,err,dot,newobj,minus,lpar,rpar,lcurly,rcurly,equ,plus,lsquare,rsquare,multiply,divide,idiv,and,or,not,pwr,comma,unaryplus,unaryminus,forloop,ifop,iffunc,ifelse,func,object,plusset, minusset, mulset, divset, idivset, setvar,my,colon,apo,mdot};

	parser();
	void init(string_view expr)	{_content = expr; set_state(0);}
	token get_token()			{return _token;}
	value_t get_value()			{return _value;}
	string_t get_name()			{return _name;}
	state get_state() const		{return _lastpos;}
	void set_state(state state)	{_pos = state; _token=end; next();}
//	tstring GetContent(State begin, State end)	{return _content.substr(begin, end-begin);}
	void check_pair(token token);
	token next();
private:
	typedef std::unordered_map<string_t, token> Keywords;
	static Keywords	_keywords;
	char_t			_decpt = std::use_facet<std::numpunct<char_t>>(std::locale()).decimal_point();
	token			_token;
	string_t		_content;
	state			_pos = 0;
	state			_lastpos = 0;
	value_t			_value;
	string_t		_name;

	char_t peek()			{return _pos >= _content.length() ? 0 : _content[_pos];}
	char_t read()			{char_t c = peek();_pos++;return c;}
	void back()				{_pos--;}
	void read_string(char_t quote);
	void read_number(char_t c);
	void read_date(char_t c);
	void read_name(char_t c);
};

// Main class for executing scripts
class NScript
{
public:
	//NScript(const tchar* script = NULL, const Context *pcontext = NULL) : _context(pcontext)	{_parser.Init(script);}
	NScript() : _context(nullptr)	{}
	~NScript(void)						{};
	value_t eval(string_view script) { std::error_code ec; return eval(script, ec); };
	value_t eval(string_view script, std::error_code& ec);
	void add(string_t name, value_t object)	{ _context.set(name, object); }

protected:
//	friend class Class; 
//	friend class Function;
	enum Precedence	{Script = 0,Statement,Assignment,Conditional,Logical,Binary,Equality,Relation,Addition,Multiplication,Power,Unary,Functional,Primary,Term};

	template <Precedence L> void parse(value_t& result, bool skip);
	template <Precedence L> void parse(parser::state state, value_t& result);
	template <Precedence L> void parse_if(value_t& result, bool skip);
	/*
	void ParseForLoop(variant_t& result, bool skip);
	void ParseArgList(args_list& args, bool forceArgs);
	void ParseFunc(variant_t& args, bool skip);
	void ParseObj(variant_t& result, bool skip);*/
	template <Precedence L, class OP> bool apply_op(OP op, value_t& result, bool skip);

	parser				_parser;
	context				_context;
	context::var_names	_varnames;

};

// Generic implementation of i_object interface
class object : public i_object	{
public:
	object()	{};
	value_t create() const				{ throw std::errc::not_supported; }
	value_t get()						{ throw std::errc::not_supported; }
	void set(value_t value)				{ throw std::errc::not_supported; }
	value_t call(value_t params)		{ throw std::errc::not_supported; }
	value_t item(value_t item)			{ throw std::errc::not_supported; }
	value_t index(value_t index)		{ throw std::errc::not_supported; }
	string_t print() const				{ throw std::errc::not_supported; }
	virtual ~object()	{};
};

/*// User-defined functions
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
*/
}

namespace std { template<> struct is_error_code_enum<nscript3::nscript_error> : public true_type {}; }
