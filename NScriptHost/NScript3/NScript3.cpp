#include "stdafx.h"
#include <ctime>
#include <iomanip>
#include <locale>
#include <limits>
#include <algorithm>
#include <functional>
#include <sstream>
#include <tuple>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "nscript3.h"
#include "noperators.h"

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

bool is_empty(const value_t& v) { return v.index() == 0; }
tm date2tm(date_t date)
{
	auto t = std::chrono::system_clock::to_time_t(date);
	tm tm;
	localtime_s(&tm, &t);
	return tm;
}

class narray;
narray* to_array_if(const value_t& v)
{
	if(auto pobj = std::get_if<object_ptr>(&v); pobj)
		if(auto parr = std::dynamic_pointer_cast<narray>(*pobj); parr)	return parr.get();
	return nullptr;
}

std::shared_ptr<narray> to_array(const value_t& v)
{
	if(auto pobj = std::get_if<object_ptr>(&v); pobj)
		if(auto parr = std::dynamic_pointer_cast<narray>(*pobj); parr)	return parr;
	return is_empty(v) ? std::make_shared<narray>() : std::make_shared<narray>(v);
}

std::string to_string(value_t v)
{
	struct print_value {
		string operator() (std::monostate)	{ return ""; }
		string operator() (int i) { return std::to_string(i); }
		string operator() (double d) {
			std::stringstream ss;
			ss << d;
			return ss.str();
		}
		string operator() (string s) { return s; }
		string operator() (nscript3::date_t dt) {
			auto tm = date2tm(dt);
			std::stringstream ss;
			bool is_date = !(tm.tm_mday == 1 && tm.tm_mon == 0 && tm.tm_year == 70);
			bool is_time = tm.tm_hour || tm.tm_min || tm.tm_sec;
			if(is_date)		ss << std::put_time(&tm, "%d.%m.%Y");
			if(is_time)		ss << (is_date ? " " : "") << std::put_time(&tm, tm.tm_sec ? "%H:%M:%S" : "%H:%M");
			return ss.str();
		}
		string operator() (nscript3::object_ptr o) { 
			auto v = o->get(); 
			if(auto po = std::get_if<object_ptr>(&v); *po != o)	return to_string(v); 
			return o->print();
		}
	};

	return std::visit(print_value(), v);
}

int to_int(value_t v)
{
	struct to_int_t {
		int operator() (std::monostate)	{ return 0; }
		int operator() (int i)			{ return i; }
		int operator() (double d)		{ return (int)(d + 0.5); }
		int operator() (string s)		{ return std::stoi( s ); }
		int operator() (date_t dt)		{ throw nscript_error::type_mismatch; }
		int operator() (object_ptr o)	{ throw nscript_error::type_mismatch; }
	};
	return std::visit(to_int_t(), v);
}

double to_double(value_t v)
{
	struct to_double_t {
		double operator() (std::monostate) { return 0.; }
		double operator() (int i) { return i; }
		double operator() (double d) { return d; }
		double operator() (string s) { return std::stod(s); }
		double operator() (date_t dt) { throw nscript_error::type_mismatch; }
		double operator() (object_ptr o) { throw nscript_error::type_mismatch; }
	};
	return std::visit(to_double_t(), v);
}

date_t to_date(const string& s)
{
	enum date_stage { day = 0, mon, year, hour, min, sec } stage = day;
	int date[6] = { 0 };
	for(auto c : s) {
		if(isdigit(c)) {
			date[stage] = date[stage] * 10 + (c - '0');
		} else if(c == '.') {
			if(stage > year)					throw nscript_error::syntax_error;
			stage = date_stage(stage + 1);
		} else if(c == ':') {
			if(stage == day)	date[3] = date[0], date[0] = 1, date[1] = 1, date[2] = 1970, stage = hour;
			if(stage < hour || stage == sec)	throw nscript_error::syntax_error;
			stage = date_stage(stage + 1);
		} else if(c == ' ') {
			if(stage != year && stage != hour)	throw nscript_error::syntax_error;
			stage = hour;
		} else	break;
	};
	if(date[2] < 100)	date[2] += date[2] < 50 ? 2000 : 1900;
	if(date[0] <= 0 || date[0] > 31)		throw nscript_error::syntax_error;
	if(date[1] <= 0 || date[1] > 12)		throw nscript_error::syntax_error;
	if(date[2] < 1900 || date[2] > 9999)	throw nscript_error::syntax_error;
	if(date[3] < 0 || date[3] > 23)			throw nscript_error::syntax_error;
	if(date[4] < 0 || date[4] > 59)			throw nscript_error::syntax_error;
	if(date[5] < 0 || date[5] > 59)			throw nscript_error::syntax_error;
	tm tm;
	tm.tm_mday = date[0];
	tm.tm_mon = date[1] - 1;
	tm.tm_year = date[2] - 1900;
	tm.tm_hour = date[3];
	tm.tm_min = date[4];
	tm.tm_sec = date[5];
	auto time = std::mktime(&tm);
	return std::chrono::system_clock::from_time_t(time);
}

date_t to_date(value_t v)
{
	struct to_date_t {
		date_t operator() (std::monostate) { throw nscript_error::type_mismatch; }
		date_t operator() (int) { throw nscript_error::type_mismatch; }
		date_t operator() (double) { throw nscript_error::type_mismatch; }
		date_t operator() (string s) { return to_date(s); }
		date_t operator() (date_t dt) { return dt; }
		date_t operator() (object_ptr) { throw nscript_error::type_mismatch; }
	};
	return std::visit(to_date_t(), v);
}

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

// Class that represents script variables
class variable : public object, public std::enable_shared_from_this<variable> {
	value_t			_value;
public:
	variable() : _value() {}
	value_t get() { return _value; }
	void set(value_t value) { _value = value; }
	value_t create() const { return std::get<object_ptr>(_value)->create(); }
	value_t call(value_t params) { return std::get<object_ptr>(_value)->call(params); }
	value_t item(value_t item) { return std::get<object_ptr>(_value)->item(item); }
	value_t index(value_t index) { 
		if(auto pobj = std::get_if<object_ptr>(&_value); pobj)	return (*pobj)->index(index);
		if(index == value_t{ 0 })	return { shared_from_this() };
		throw std::errc::invalid_argument;
	}
	string_t print() const { return std::get<object_ptr>(_value)->print(); }
};


// Class that represents arrays
class narray : public object, public std::enable_shared_from_this<narray> {
	std::vector<value_t>	_items;
public:
	narray() {}
	template<class InputIt> narray(InputIt first, InputIt last) : _items(first, last) {}
	narray(value_t val) : _items { val } {}
	value_t get() {
		if(_items.size() == 1) return _items.front(); 
		return { std::static_pointer_cast<i_object>(shared_from_this()) };
	}
	value_t index(value_t index) {
		if(auto pi = std::get_if<int>(&index))	return std::make_shared<indexer>(shared_from_this(), *pi);
		throw std::errc::invalid_argument;
	}
	string_t print() const { 
		std::stringstream ss;
		ss << '[';
		std::stringstream::pos_type pos = 0;
		for(auto& v : _items) { 
			if(ss.tellp() > 1) ss << "; "; 
			ss << to_string(v);
		}
		ss << ']';
		return ss.str();
	}
	void push_back(const value_t& val)	{ _items.push_back(val); }
	void push_back(value_t&& val)		{ _items.push_back(std::forward<value_t>(val)); }
	std::vector<value_t>& items()		{ return _items; }

protected:
	class indexer : public object {
		std::shared_ptr<narray>	_data;
		int						_index;
	public:
		indexer(std::shared_ptr<narray> arr, int index) : _index(index), _data(arr) {};
		value_t get()			{ return _data->items()[_index]; }
		void set(value_t value) { _data->items()[_index] = value; }
		value_t call(value_t params)	{ return std::visit(op_call(), _data->items()[_index], params); }
		//value_t item(value_t item)		{ return std::visit(op_item(), _data->items()[_index], item); }
		value_t index(value_t index)	{ return std::visit(op_index(), _data->items()[_index], index); }
	};
};

// Built-in functions
template<class FN> class builtin_function : public object {
public:
	builtin_function(int count, FN func) : _count(count), _func(func) {}
	value_t call(value_t params) {
		if(auto pa = to_array_if(params); pa) {
			if(_count >= 0 && _count != pa->items().size())	throw nscript_error::bad_param_count;
			return _func(pa->items());
		} 
		else if(is_empty(params) && _count <= 0)	return _func({});
		else if(_count < 0 || _count == 1)			return _func({ params });
		else										throw nscript_error::bad_param_count;	
	}
protected:
	const int			_count;
	FN					_func;
};
template<class FN> object_ptr make_fn(int count, FN fn) { return std::make_shared<builtin_function<FN>>(count, fn); }

// Context
struct copy_var {
	const context	*_from;
	context			*_to;
	copy_var(const context *from ,context *to) : _from(from), _to(to)	{};
	void operator() (const string_t& name) { /*_to->set(_from->get(name));*/ }
};

context::vars_t	context::_globals {
	{ "empty",	value_t()},
	{ "true",	{ -1} },
	{ "false",	{ 0} },
	{ "int",	make_fn(1, [](const params_t& args) { return to_int(args.front()); }) },
	{ "dbl",	make_fn(1, [](const params_t& args) { return to_double(args.front()); }) },
	{ "str",	make_fn(1, [](const params_t& args) { return to_string(args.front()); }) },
	{ "date",	make_fn(1, [](const params_t& args) { return to_date(args.front()); }) },
	// Date
	{ "now",	make_fn(0, [](const params_t& args) { return std::chrono::system_clock::now(); }) },
	{ "day",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_mday; }) },
	{ "month",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_mon + 1; }) },
	{ "year",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_year + 1900; }) },
	{ "hour",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_hour; }) },
	{ "minute",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_min; }) },
	{ "second",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_sec; }) },
	{ "dayofweek",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_wday; }) },
	{ "dayofyear",	make_fn(1, [](const params_t& args) { return date2tm(to_date(args.front())).tm_yday + 1; }) },
	// Math
	{ "pi",		make_fn(0, [](const params_t& args) { return 3.14159265358979323846; }) },
	{ "rnd",	make_fn(0, [](const params_t& args) { return (double)rand() / (double)RAND_MAX; }) },
	{ "sin",	make_fn(1, [](const params_t& args) { return sin(to_double(args.front())); }) },
	{ "cos",	make_fn(1, [](const params_t& args) { return cos(to_double(args.front())); }) },
	{ "tan",	make_fn(1, [](const params_t& args) { return tan(to_double(args.front())); }) },
	{ "atan",	make_fn(1, [](const params_t& args) { return atan(to_double(args.front())); }) },
	{ "abs",	make_fn(1, [](const params_t& args) { return fabs(to_double(args.front())); }) },
	{ "exp",	make_fn(1, [](const params_t& args) { return exp(to_double(args.front())); }) },
	{ "log",	make_fn(1, [](const params_t& args) { return log(to_double(args.front())); }) },
	{ "sqr",	make_fn(1, [](const params_t& args) { return sqrt(to_double(args.front())); }) },
	{ "sqrt",	make_fn(1, [](const params_t& args) { return sqrt(to_double(args.front())); }) },
	{ "atan2",	make_fn(2, [](const params_t& args) { return atan2(to_double(args[0]), to_double(args[1])); }) },
	{ "sgn",	make_fn(1, [](const params_t& args) { double d = to_double(args.front()); return d < 0 ? -1 : d > 0 ? 1 : 0; }) },
	{ "fract",	make_fn(1, [](const params_t& args) { return to_double(args.front()) - to_int(args.front()); }) },
	// String
	{ "chr",	make_fn(1, [](const params_t& args) { return string_t(1, (string_t::value_type)to_int(args.front()) ); }) },
	{ "asc",	make_fn(1, [](const params_t& args) { return (int)to_string(args.front()).c_str()[0]; }) },
	{ "len",	make_fn(1, [](const params_t& args) { return (int)to_string(args.front()).size(); }) },
	{ "left",	make_fn(2, [](const params_t& args) { return to_string(args[0]).substr(0, to_int(args[1])); }) },
	{ "right",	make_fn(2, [](const params_t& args) { auto s = to_string(args[0]); auto n = to_int(args[1]); return s.substr(s.size() - n, n); }) },
	{ "mid",	make_fn(3, [](const params_t& args) { return to_string(args[0]).substr(to_int(args[1]), to_int(args[2])); }) },
	{ "upper",	make_fn(1, [](const params_t& args) { auto s = to_string(args[0]); return std::transform(s.begin(), s.end(), s.begin(), ::toupper), s; }) },
	{ "lower",	make_fn(1, [](const params_t& args) { auto s = to_string(args[0]); return std::transform(s.begin(), s.end(), s.begin(), ::tolower), s; }) },
	{ "string",	make_fn(2, [](const params_t& args) { return string_t(to_int(args[0]), *to_string(args[1]).c_str()); }) },
	{ "replace",make_fn(3, [](const params_t& args) { 
		string_t s(to_string(args[0])), from(to_string(args[1])), to(to_string(args[2]));
		for(string_t::size_type p = 0; (p = s.find(from, p)) != string_t::npos; p += to.size())	s.replace(p, from.size(), to);
		return s;
	}) },
	{ "instr",	make_fn(2, [](const params_t& args) { return (int)to_string(args[0]).find(to_string(args[1])); }) },
	//{ "format",	make_fn(1, [](const params_t& args) { std::stringstream str; str << std::hex << to_int(argv[0]); return str.str(); }) },
	{ "hex",	make_fn(1, [](const params_t& args) { std::stringstream str; str << std::hex << to_int(args.front()); return str.str(); }) },
	{ "rgb",	make_fn(3, [](const params_t& args) { return to_int(args[2]) * 65536 + to_int(args[1]) * 256 + to_int(args[0]); }) },
	// Array
	{ "size",	make_fn(-1, [](const params_t& args){ return (int)args.size(); }) },
	{ "add",	make_fn(2, [](const params_t& args) { auto a = to_array(args[0]); return a->items().push_back(args[1]), a; }) },
	{ "remove",	make_fn(2, [](const params_t& args) { auto a = to_array(args[0]); return a->items().erase( a->items().begin() + to_int(args[1])), a; }) },
	{ "min",	make_fn(-1, [](const params_t& args) { auto pe = std::min_element(begin(args), end(args), std::less<nscript3::value_t>()); return pe == end(args) ? value_t{} : *pe; }) },
	{ "max",	make_fn(-1, [](const params_t& args) { auto pe = std::max_element(begin(args), end(args), std::less<nscript3::value_t>()); return pe == end(args) ? value_t{} : *pe; }) },
	{ "fold",	make_fn(1, [](const params_t& args) { return 0; }) },
	{ "map",	make_fn(1, [](const params_t& args) { return 0; }) },
	{ "filter",	make_fn(1, [](const params_t& args) { return 0; }) },
	{ "head",	make_fn(-1, [](const params_t& args) { return args.empty() ? value_t{} : args.front(); }) },
	{ "tail",	make_fn(-1, [](const params_t& args) { return args.empty() ? value_t{} : std::make_shared<narray>(args.begin() + 1, args.end()); }) },

/*
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
	{ TEXT("max"),		new BuiltinFunction(-1, FnFind<VARCMP_GT>) },*/
};

context::context(const context *base, const var_names *vars) : _locals(1)
{
	if(base) {
		if(vars)	std::for_each(vars->begin(), vars->end(), copy_var(base, this));
		else		_locals.assign(base->_locals.begin(), base->_locals.end());
		//_locals[0] = base->_locals[0];
	}
}

value_t& context::get(string_t name, bool local)
{
	if(!local)	{
		for(int i = (int)_locals.size() - 1; i >= -1; i--)	{
			vars_t& plane = i < 0 ? _globals : _locals[i];
			if(auto p = plane.find(name); p != plane.end()) return	p->second;
		}
	}
	return _locals.back()[name] = std::make_shared<variable>();
}

std::optional<value_t> context::get(string_t name) const
{
	for(auto ri = _locals.rbegin(); ri != _locals.rend(); ri++)	{
		if(auto p = ri->find(name);  p != ri->end())	return { p->second };
	}
	return {};
}

static auto s_operators = std::make_tuple(
	std::tuple<op_statmt>(),
	std::tuple<>(),
	std::tuple<op_assign, op_addset, op_subset, op_mulset, op_divset>(),
	std::tuple<op_if>(),
	std::tuple<op_land, op_lor>(),
	std::tuple<op_and, op_or>(),
	std::tuple<op_eq, op_ne>(),
	std::tuple<op_gt, op_ge, op_lt, op_le>(),

	std::tuple<op_add, op_sub>(),
	std::tuple<op_mul, op_div, op_mod>(),
	std::tuple<op_pow>(),
	std::tuple<op_ppx, op_mmx, op_neg, op_not, op_lnot>(),
	std::tuple<op_xpp, op_xmm, op_call, op_index>()
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
	catch(std::errc e)		 { ec = make_error_code(e); }
	_context.pop();
	return result;
}

// Parse "if <cond> <true-part> [else <part>]" statement
template<NScript::Precedence level> void NScript::parse_if(value_t& result, bool skip)	{
	bool cond = skip || to_int(result);
	parse<level>(result, !cond || skip);
	if(_parser.get_token() == parser::ifelse || _parser.get_token() == parser::colon)	{
		_parser.next();
		parse<level>(result, cond || skip);
	}
}
/*
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
}*/

// Jump to position <state>, parse expression and return back
template <NScript::Precedence level> void NScript::parse(parser::state state, value_t& result)
{
	auto current = _parser.get_state();
	_parser.set_state(state);
	parse(level, result, false);
	_parser.set_state(current);
}

template <NScript::Precedence level, class OP> bool NScript::apply_op(OP op, value_t& result, bool skip)
{
	if(op.token == _parser.get_token()) {
		if(op.token != parser::lpar && op.token != parser::lsquare && op.token != parser::dot)	_parser.next();
		if(op.token == parser::ifop) {		// ternary "a?b:c" operator
			parse_if<Logical>(result, skip);
			return true;
		}

		value_t right;

		// parse right-hand operand
		if(op.token == parser::dot) { right = _parser.get_value(); _parser.next(); }	// special case for '.' operator
		else if(op.assoc == associativity::right)	parse<level>(right, skip);			// right-associative operators
		else if(op.token != parser::unaryplus && op.token != parser::unaryminus  && op.token != parser::apo && !(level == Script && _parser.get_token() == parser::rcurly))
			parse<level+1>(right, skip);												// left-associative operators

		if(op.deref == dereference::left  || op.deref == dereference::both)	*result;
		if(op.deref == dereference::right || op.deref == dereference::both)	*right;

		if(!skip)	result = std::visit(op, result, right);								// perform operator's action
		return true;
	}
	return false;
}


template <NScript::Precedence level> void NScript::parse(value_t& result, bool skip)
{
	// main parse loop
	parser::token token = _parser.get_token();

	// parse left-hand operand (for binary operators)
	parse<(Precedence)(level + 1)>(result, skip);
	if(_parser.get_token() == parser::end)	return;
	while(std::apply([&](auto ...op) { return (apply_op<level, decltype(op)>(op, result, skip) || ...); }, std::get<level>(s_operators)));
}

template<> void NScript::parse<NScript::Unary>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	if(_parser.get_token() == parser::end)	return;
	if(!std::apply([&](auto ...op) { return (apply_op<NScript::Unary, decltype(op)>(op, result, skip) || ...); }, std::get<NScript::Unary>(s_operators)))
		parse<NScript::Functional>(result, skip);
}

template<> void NScript::parse<NScript::Statement>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	parse<Assignment>(result, skip);
	if(_parser.get_token() == parser::comma) {
		auto a = std::make_shared<narray>(*result);
		do {
			value_t v;
			_parser.next();
			parse<Assignment>(v, skip);
			a->push_back(*v);
		} while(_parser.get_token() == parser::comma);
		result = a;
	}
}

template<> void NScript::parse<NScript::Primary>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	bool local = false;
	switch(token) {
	case parser::value:		if(!skip)	result = _parser.get_value(); _parser.next(); break;
	case parser::my:
		if(_parser.next() != parser::name)	throw nscript_error::syntax_error;
		local = true;
		[[fallthrough]];
	case parser::name:
		if(skip)	_varnames.insert(_parser.get_name());
		else		result = _context.get(_parser.get_name(), local);
		_parser.next();
		break;
	case parser::iffunc:	_parser.next(); parse<Assignment>(result, skip); parse_if<Assignment>(result, skip); break;
/*		case Parser::forloop:	ParseForLoop(result, skip);break;
		case Parser::idiv:		ParseFunc(result, skip); break;
		case Parser::func:		ParseFunc(result, skip);break;
		case Parser::object:	ParseObj(result, skip);break;*/
	case parser::lpar:
	case parser::lsquare:
		_parser.next();
		if(!skip)	result = value_t{};
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
		case '#':	_token = value; read_string(c); _value = to_date(_value); break;
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
	enum date_stage { day = 0, mon, year, hour, min, sec } stage = day;
	int date[6] = { 0 };
	char_t c;
	while(c = read()) {
		if(isdigit(c)) {
			date[stage] = date[stage] * 10 + (c - '0');
		} else if(c == '.') {
			if(stage > year)					throw nscript_error::syntax_error;
			stage = date_stage(stage + 1);
		} else if(c == ':') {
			if(stage < hour || stage == sec)	throw nscript_error::syntax_error;
			stage = date_stage(stage + 1);
		} else if(c == ' ') {
			if(stage != year && stage != hour)	throw nscript_error::syntax_error;
			stage = hour;
		} else	break;
	};
	if(c != quote)							throw nscript_error::syntax_error;
	if(date[2] < 100)	date[2] += date[2] < 50 ? 2000 : 1900;
	if(date[0] <= 0 || date[0] > 31)		throw nscript_error::syntax_error;
	if(date[1] <= 0 || date[1] > 12)		throw nscript_error::syntax_error;
	if(date[2] < 1900 || date[2] > 9999)	throw nscript_error::syntax_error;
	if(date[3] < 0 || date[3] > 23)			throw nscript_error::syntax_error;
	if(date[4] < 0 || date[4] > 59)			throw nscript_error::syntax_error;
	if(date[5] < 0 || date[5] > 59)			throw nscript_error::syntax_error;
	tm tm;
	tm.tm_mday = date[0];	
	tm.tm_mon = date[1] - 1;
	tm.tm_year = date[2] - 1900;
	tm.tm_hour = date[3];
	tm.tm_min = date[4];
	tm.tm_sec = date[5];
	auto time = std::mktime(&tm);
	_value = std::chrono::system_clock::from_time_t(time);
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