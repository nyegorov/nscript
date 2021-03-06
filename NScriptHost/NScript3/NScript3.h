// NScript - lightweight, single-pass parser for executing c-style expressions. 
// Supports integer, double, string and date types. Has limited set of flow 
// control statements (if/then/else, loops). Fully extensible by user-defined 
// objects with i_object interface.

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace nscript3	{

enum class errc {runtime_error = 1001, unexpected_eof, missing_character, unknown_var, missing_lval, syntax_error, bad_param_count, type_mismatch };

class nscript_category_impl : public std::error_category
{
public:
	virtual const char * name() const noexcept { return "nscript"; }
	virtual std::string message(int ev) const {
		switch(static_cast<errc>(ev))	{
		case errc::runtime_error:		return "runtime error";
		case errc::unexpected_eof:		return "unexpected end";
		case errc::missing_character:	return "missing character";
		case errc::missing_lval:		return "missing variable";
		case errc::unknown_var:			return "unknown variable";
		case errc::syntax_error:		return "syntax error";
		case errc::bad_param_count:		return "bad parameters count";
		case errc::type_mismatch:		return "type mismatch";
		default:						return "unknown error";
		}
	};
};

const std::error_category& nscript_category();
std::error_code make_error_code(errc e);
std::error_condition make_error_condition(errc e);

struct i_object;
class v_array;
using std::string_view;
using string_t = std::string;
using object_ptr = std::shared_ptr<i_object>;
using array_ptr = std::shared_ptr<v_array>;
using value_t = std::variant<object_ptr, bool, double, string_t>;
using params_t = std::vector<value_t>;

std::string to_string(value_t v);
bool to_bool(value_t v);
double to_double(value_t v);
tm to_date(value_t v);
params_t* to_array_if(const value_t& v);
params_t* to_array_if(const object_ptr& o);
array_ptr to_array(const value_t& v);
inline bool is_empty(const value_t& v) { auto po = std::get_if<object_ptr>(&v); return po && po->get() == nullptr; }

// Interface for extension objects
struct i_object {
	virtual value_t create() const = 0;					// return new <object>
	virtual value_t get()  = 0;							// return <object>
	virtual void set(value_t value) = 0;				// <object> = value
	virtual value_t call(value_t params) = 0;			// return <object>(params)
	virtual value_t item(string_t item) = 0;			// return <object>.item
	virtual value_t index(value_t index) = 0;			// return <object>[index]
	virtual string_t print() const = 0;
};

using args_list = std::vector<string_t>;

// Container for storing named objects and variables
class context	
{
public:
	typedef std::unordered_set<string_t>			var_names;
	context(const context *base, const var_names *vars = nullptr);
	void push()		{_locals.emplace_back();}
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
	using state = size_t;
	enum token	{end,mod,assign,ge,gt,le,lt,nequ,name,value,land,lor,lnot,stmt,err,dot,newobj,minus,lpar,rpar,lcurly,rcurly,equ,plus,lsquare,rsquare,multiply,divide,lambda,and,or,not,pwr,comma,unaryplus,unaryminus,forloop,ifop,iffunc,ifelse,func,object,plusset, minusset, mulset, divset, idivset, setvar,my,colon,apo,mdot};

	parser();
	void init(string_view expr)	{if(!expr.empty()) _content = expr; set_state(0);}
	token get_token()			{return _token;}
	value_t get_value()			{return _value;}
	string_t get_name()			{return _name;}
	state get_state() const		{return _lastpos;}
	void set_state(state state)	{_pos = state; _token=end; next();}
	string_t get_content(state begin, state end)	{return _content.substr(begin, end-begin);}
	void check_pair(token token);
	token next();
private:
	int			_decpt = std::use_facet<std::numpunct<char>>(std::locale()).decimal_point();
	token		_token;
	string_t	_content;
	state		_pos = 0;
	state		_lastpos = 0;
	value_t		_value;
	string_t	_name;

	int peek()			{ if(_pos >= _content.length())	return 0; int c = _content[_pos]; return c < 0 ? c + 256 : c; }
	int read()			{auto c = peek(); _pos++; return c;}
	void back()				{_pos--;}
	void read_string(int quote);
	void read_number(int c);
	void read_name(int c);
};


struct error_info {
	std::error_code	code;
	string_t		content;
	size_t			position;
};

// Main class for executing scripts
class nscript
{
public:
	nscript(string_view script, const context *pcontext = nullptr) : _context(pcontext)	{_parser.init(script);}
	nscript() : _context(nullptr)	{}
	~nscript(void)					{};
	std::tuple<bool, value_t> eval(string_view script);
	void add(string_t name, value_t object)	{ _context.set(name, object); }
	error_info get_error_info() { return { _last_error, _parser.get_content(0, -1), _parser.get_state() }; }

protected:
	enum Precedence	{Script = 0,Statement,Assignment,Conditional,Logical,Binary,Equality,Relation,Addition,Multiplication,Power,Unary,Functional,Primary,Term};
	friend class user_class;
	friend class user_function;

	template <Precedence> void parse(value_t& result, bool skip);
	template <Precedence> void parse(parser::state state, value_t& result);
	template <Precedence> void parse_if(value_t& result, bool skip);
	void parse_args(args_list& args);
	void parse_func(value_t& result, bool skip);
	void parse_for(value_t& result, bool skip);
	void parse_obj(value_t& result, bool skip);
	template <Precedence, class OP> bool apply_op(OP op, value_t& result, bool skip);

	parser				_parser;

	context				_context;
	context::var_names	_varnames;
	std::error_code		_last_error;
};

// Generic implementation of i_object interface
class object : public i_object, public std::enable_shared_from_this<object> {
public:
	object()	{};
	value_t create() const				{ throw std::system_error(std::make_error_code(std::errc::not_supported), "object"); }
	value_t get()						{ return shared_from_this(); }
	void set(value_t value)				{ throw std::system_error(std::make_error_code(std::errc::not_supported), "object"); }
	value_t call(value_t params)		{ throw std::system_error(std::make_error_code(std::errc::not_supported), "object"); }
	value_t item(string_t item)			{ throw std::system_error(std::make_error_code(std::errc::not_supported), "object"); }
	value_t index(value_t index)		{ throw std::system_error(std::make_error_code(std::errc::not_supported), "object"); }
	string_t print() const				{ return "[object]"; }
	virtual ~object()					{};
};

}

namespace std { template<> struct is_error_code_enum<nscript3::errc> : public true_type {}; }
