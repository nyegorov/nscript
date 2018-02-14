#include "stdafx.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "nscript3.h"
#include "nobjects.h"
#include "noperators.h"

#undef min
#undef max

template<> struct std::less<nscript3::value_t> {
	bool operator()(const nscript3::value_t &v1, const nscript3::value_t& v2) { return std::visit(nscript3::comparator(), v1, v2) < 0; }
};

namespace nscript3	{

// Helper functions

const std::error_category& nscript_category()
{
	static nscript_category_impl instance;
	return instance;
}

std::error_code make_error_code(errc e)				{ return std::error_code(static_cast<int>(e), nscript_category()); }
std::error_condition make_error_condition(errc e)	{ return std::error_condition(static_cast<int>(e), nscript_category()); }

string_t tm2str(tm tm) {
	int is_date = !(tm.tm_mday == 1 && tm.tm_mon == 0 && tm.tm_year == 70) ? 1 : 0;
	int is_time = tm.tm_hour || tm.tm_min || tm.tm_sec ? 1 : 0;
	int is_sec  = tm.tm_sec ? 1 : 0;
	char *format[8] = { "", "", "%d:%02d" , "%d:%02d:%02d", "%02d.%02d.%04d", "%02d.%02d.%04d", "%02d.%02d.%04d %d:%02d", "%02d.%02d.%04d %d:%02d:%02d"};
	char buf[32];
	sprintf_s(buf, format[ is_date * 4 + is_time * 2 + is_sec ], tm.tm_mday, 1 + tm.tm_mon, 1900 + tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);
	return buf;
}

params_t* to_array_if(const object_ptr& o)
{
	if(auto pa = std::dynamic_pointer_cast<v_array>(o); pa)	return &pa->items();
	return nullptr;
}

params_t* to_array_if(const value_t& v)
{
	if(auto pobj = std::get_if<object_ptr>(&v); pobj)	return to_array_if(*pobj);
	return nullptr;
}

array_ptr to_array(const value_t& v)
{
	if(auto pobj = std::get_if<object_ptr>(&v); pobj && pobj->get())
		if(auto parr = std::dynamic_pointer_cast<v_array>(*pobj); parr)	return parr;
	return is_empty(v) ? std::make_shared<v_array>() 
					   : std::make_shared<v_array>(std::initializer_list<value_t>{ v });
}

// 'dereference' object. If v holds an ext. object, replace it with the value of object
value_t& operator *(value_t& v)	{
	if(auto pobj = std::get_if<object_ptr>(&v); pobj && pobj->get()) {
		v = (*pobj)->get();
	}
	return v;
}

class context_scope
{
	context& _ctx;
public:
	context_scope(context& ctx) : _ctx(ctx) { _ctx.push(); }
	~context_scope()						{ _ctx.pop(); }
};

#pragma region Context
context::vars_t	context::_globals {
	{ "empty",	value_t()},
	{ "hash",	std::make_shared<assoc_array>() },
	{ "true",	{ true } },
	{ "false",	{ false } },
	{ "bool",	make_fn(1, [](const params_t& args) { return to_bool(args.front()); }) },
	{ "int",	make_fn(1, [](const params_t& args) { return double((int)to_double(args.front())); }) },
	{ "dbl",	make_fn(1, [](const params_t& args) { return to_double(args.front()); }) },
	{ "str",	make_fn(1, [](const params_t& args) { return to_string(args.front()); }) },
	{ "date",	make_fn(1, [](const params_t& args) { return tm2str(to_date(args.front())); }) },
	// Date
	{ "now",	make_fn(0, [](const params_t& args) { return tm2str(date2tm(std::chrono::system_clock::now())); }) },
	{ "day",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_mday); }) },
	{ "month",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_mon + 1); }) },
	{ "year",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_year + 1900); }) },
	{ "hour",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_hour); }) },
	{ "minute",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_min); }) },
	{ "second",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_sec); }) },
	{ "dayofweek",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_wday); }) },
	{ "dayofyear",	make_fn(1, [](const params_t& args) { return double(to_date(args.front()).tm_yday + 1); }) },
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
	{ "atan2",	make_fn(2, [](const params_t& args) { auto x = to_double(args[0]), y = to_double(args[1]); return atan2(x, y); }) },
	{ "sgn",	make_fn(1, [](const params_t& args) { double d = to_double(args.front()); return d < 0 ? -1. : d > 0 ? 1. : 0.; }) },
	{ "fract",	make_fn(1, [](const params_t& args) { auto d = to_double(args.front()); return d - (int)d; }) },
	// String
	{ "chr",	make_fn(1, [](const params_t& args) { return string_t(1, (string_t::value_type)to_double(args.front()) ); }) },
	{ "asc",	make_fn(1, [](const params_t& args) { return (double)to_string(args.front()).c_str()[0]; }) },
	{ "len",	make_fn(1, [](const params_t& args) { return (double)to_string(args.front()).size(); }) },
	{ "left",	make_fn(2, [](const params_t& args) { return to_string(args[0]).substr(0, (int)to_double(args[1])); }) },
	{ "right",	make_fn(2, [](const params_t& args) { auto s = to_string(args[0]); auto n = (int)to_double(args[1]); return s.substr(s.size() - n, n); }) },
	{ "mid",	make_fn(3, [](const params_t& args) { return to_string(args[0]).substr((int)to_double(args[1]), (int)to_double(args[2])); }) },
	{ "upper",	make_fn(1, [](const params_t& args) { auto s = to_string(args[0]); return std::transform(s.begin(), s.end(), s.begin(), ::toupper), s; }) },
	{ "lower",	make_fn(1, [](const params_t& args) { auto s = to_string(args[0]); return std::transform(s.begin(), s.end(), s.begin(), ::tolower), s; }) },
	{ "string",	make_fn(2, [](const params_t& args) { return string_t((int)to_double(args[0]), *to_string(args[1]).c_str()); }) },
	{ "replace",make_fn(3, [](const params_t& args) { 
		string_t s(to_string(args[0])), from(to_string(args[1])), to(to_string(args[2]));
		for(string_t::size_type p = 0; (p = s.find(from, p)) != string_t::npos; p += to.size())	s.replace(p, from.size(), to);
		return s;
	}) },
	{ "instr",	make_fn(2, [](const params_t& args) { return (double)(int)to_string(args[0]).find(to_string(args[1])); }) },
	//{ "format",	make_fn(1, [](const params_t& args) { std::stringstream str; str << std::hex << to_int(argv[0]); return str.str(); }) },
	{ "hex",	make_fn(1, [](const params_t& args) { std::stringstream str; str << std::hex << (int)to_double(args.front()); return str.str(); }) },
	{ "rgb",	make_fn(3, [](const params_t& args) { return to_double(args[2]) * 65536 + to_double(args[1]) * 256 + to_double(args[0]); }) },
	// Array
	{ "size",	make_fn(-1, [](const params_t& args){ return (double)args.size(); }) },
	{ "add",	make_fn(2, [](const params_t& args) { auto a = to_array(args[0]); return a->items().push_back(args[1]), a; }) },
	{ "remove",	make_fn(2, [](const params_t& args) { auto a = to_array(args[0]); return a->items().erase( a->items().begin() + (int)to_double(args[1])), a; }) },
	{ "min",	make_fn(-1, [](const params_t& args) { auto pe = std::min_element(begin(args), end(args), std::less<nscript3::value_t>()); return pe == end(args) ? value_t{} : *pe; }) },
	{ "max",	make_fn(-1, [](const params_t& args) { auto pe = std::max_element(begin(args), end(args), std::less<nscript3::value_t>()); return pe == end(args) ? value_t{} : *pe; }) },
	{ "fold",	make_fn(1, [](const params_t& args) { return std::make_shared<fold_function>(std::get<object_ptr>(args[0])); }) },
	{ "map",	make_fn(1, [](const params_t& args) { return std::make_shared<map_function>(std::get<object_ptr>(args[0])); }) },
	{ "filter",	make_fn(1, [](const params_t& args) { return std::make_shared<filter_function>(std::get<object_ptr>(args[0])); }) },
	{ "head",	make_fn(-1, [](const params_t& args) { return args.empty() ? value_t{} : args.front(); }) },
	{ "tail",	make_fn(-1, [](const params_t& args) { return args.empty() ? value_t{} : std::make_shared<v_array>(args.begin() + 1, args.end()); }) },
};

context::context(const context *base, const var_names *vars) : _locals(1)
{
	if(base) {
		if(vars)	{ for(auto& v : *vars) if(auto o = base->get(v); o)	set(v, o.value()); }
		else		_locals.assign(base->_locals.begin(), base->_locals.end());
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
#pragma endregion

#pragma region Nscript

static auto s_operators = std::make_tuple(
	std::tuple<op_statmt>(),
	std::tuple<>(),
	std::tuple<op_assign, op_addset, op_subset, op_mulset, op_divset, op_join>(),
	std::tuple<op_if>(),
	std::tuple<op_land, op_lor>(),
	std::tuple<op_and, op_or>(),
	std::tuple<op_eq, op_ne>(),
	std::tuple<op_gt, op_ge, op_lt, op_le>(),

	std::tuple<op_add, op_sub>(),
	std::tuple<op_mul, op_div, op_mod>(),
	std::tuple<op_pow, op_dot>(),
	std::tuple<op_ppx, op_mmx, op_new, op_neg, op_not, op_lnot, op_head>(),
	std::tuple<op_xpp, op_xmm, op_call, op_index, op_item, op_tail>()
);

std::tuple<bool, value_t> nscript::eval(std::string_view script)
{
	value_t result;
	_last_error.clear();
	context_scope scope(_context);
	try	{
		_parser.init(script);
		parse<Script>(result, false);
		if(_parser.get_token() != parser::end)	throw std::system_error(errc::syntax_error, "eval");
		*result;
	}
	catch(std::system_error& se){ _last_error = se.code();  return { false, string_t(se.what()) }; }
	catch(std::exception& e)	{ _last_error = errc::runtime_error; return { false, string_t(e.what()) }; }
	catch(...)					{ _last_error = errc::runtime_error; return { false, {} }; }
	return { true, result };
}

// Parse comma-separated arguments list
void nscript::parse_args(args_list& args) {
	auto token = _parser.next();
	if(token == parser::lpar)	_parser.next();

	while(_parser.get_token() == parser::name) {
		args.push_back(_parser.get_name());
		if(_parser.next() != parser::comma) break;
		_parser.next();
	};
	if(args.size() == 1 && args.front() == "@")	args.clear();

	if(_parser.get_token() == parser::rpar)	_parser.next();
}

// Jump to position <state>, parse expression and return back
template <nscript::Precedence P> void nscript::parse(parser::state state, value_t& result)
{
	auto current = _parser.get_state();
	_parser.set_state(state);
	parse<P>(result, false);
	_parser.set_state(current);
}

template <nscript::Precedence P, class OP> bool nscript::apply_op(OP op, value_t& result, bool skip)
{
	if(op.token == _parser.get_token()) {
		if(op.token != parser::lpar && op.token != parser::lsquare && op.token != parser::dot)	_parser.next();
		if(op.token == parser::ifop) {		// ternary "a?b:c" operator
			parse_if<Logical>(result, skip);
			return true;
		}

		value_t right = result;

		// parse right-hand operand
		if(op.token == parser::dot) { right = _parser.get_name(); _parser.next(); }		// special case for '.' operator
		else if(op.assoc == associativity::right)	parse<P>(right, skip);				// right-associative operators
		else if(op.assoc == associativity::left)	parse<P+1>(right, skip);			// left-associative operators

		if(op.deref == dereference::left  || op.deref == dereference::both)	*result;
		if(op.deref == dereference::right || op.deref == dereference::both)	*right;

		if(!skip)	result = std::visit(op, result, right);								// perform operator's action
		return true;
	}
	return false;
}


template <nscript::Precedence P> void nscript::parse(value_t& result, bool skip)
{
	// main parse loop
	parser::token token = _parser.get_token();

	// parse left-hand operand (for binary operators)
	parse<Precedence(P + 1)>(result, skip);
	if(_parser.get_token() == parser::end)	return;
	while(std::apply([&](auto ...op) { return (apply_op<P, decltype(op)>(op, result, skip) || ...); }, std::get<P>(s_operators)));
}

template<> void nscript::parse<nscript::Unary>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	if(_parser.get_token() == parser::end)	return;
	if(!std::apply([&](auto ...op) { return (apply_op<nscript::Unary, decltype(op)>(op, result, skip) || ...); }, std::get<nscript::Unary>(s_operators)))
		parse<nscript::Functional>(result, skip);
}

template<> void nscript::parse<nscript::Statement>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	parse<Assignment>(result, skip);
	if(_parser.get_token() == parser::comma) {
		auto a = std::make_shared<v_array>(std::initializer_list<value_t>{*result});
		do {
			value_t v;
			_parser.next();
			parse<Assignment>(v, skip);
			a->items().push_back(*v);
		} while(_parser.get_token() == parser::comma);
		result = a;
	}
}

template<> void nscript::parse<nscript::Primary>(value_t& result, bool skip)
{
	parser::token token = _parser.get_token();
	bool local = false;
	switch(token) {
	case parser::value:		if(!skip)	result = _parser.get_value(); _parser.next(); break;
	case parser::my:
		if(_parser.next() != parser::name)	throw std::system_error(errc::syntax_error, "'my'");
		local = true;
		[[fallthrough]];
	case parser::name:
		if(skip)	_varnames.insert(_parser.get_name());
		else		result = _context.get(_parser.get_name(), local);
		_parser.next();
		break;
	case parser::iffunc:	_parser.next(); parse<Assignment>(result, skip); parse_if<Assignment>(result, skip); break;
	case parser::lambda:
	case parser::func:		parse_func(result, skip); break;
	case parser::forloop:	parse_for(result, skip); break;
	case parser::object:	parse_obj(result, skip);break;
	case parser::lpar:
	case parser::lsquare:
		_parser.next();
		if(!skip)	result = value_t{};
		parse<Statement>(result, skip);
		_parser.check_pair(token);
		break;
	case parser::lcurly:
		_parser.next();
		{	
			context_scope scope(_context);
			parse<Script>(result, skip);
		}
		_parser.check_pair(token);
		break;
	case parser::end:		throw std::system_error(errc::unexpected_eof);
	}
}

// Parse "if <cond> <true-part> [else <part>]" statement
template<nscript::Precedence P> void nscript::parse_if(value_t& result, bool skip) {
	bool cond = skip || to_bool(*result);
	parse<P>(result, !cond || skip);
	if(_parser.get_token() == parser::ifelse || _parser.get_token() == parser::colon) {
		_parser.next();
		parse<P>(result, cond || skip);
	}
}

// Parse "for([<start>];<cond>;[<inc>])	<body>" statement
void nscript::parse_for(value_t& result, bool skip) {
	parser::state condition, increment, body;
	_parser.next();
	if(_parser.get_token() != parser::lpar)		throw std::system_error(errc::syntax_error, "'for'");
	_parser.next();
	if(_parser.get_token() != parser::stmt) {	// start expression
		parse<Statement>(result, skip);
		if(_parser.get_token() != parser::stmt)	throw std::system_error(errc::syntax_error, "'for'");
	}
	_parser.next();
	condition = _parser.get_state();
	if(_parser.get_token() != parser::stmt)	{	// exit condition
		parse<Statement>(result, true);
		if(_parser.get_token() != parser::stmt)	throw std::system_error(errc::syntax_error, "'for'");
	}
	_parser.next();
	increment = _parser.get_state();
	if(_parser.get_token() != parser::rpar)	{	// increment
		parse<Statement>(result, true);
		if(_parser.get_token() != parser::rpar)	throw std::system_error(errc::syntax_error, "'for'");
	}
	_parser.next();
	body = _parser.get_state();
	parse<Statement>(result, true);				// body
	if(!skip)	{
		while(true)	{
			parse<Statement>(condition, result);
			if(!to_bool(*result))	break;
			parse<Statement>(body, result);
			parse<Statement>(increment, result);
		}
	}
}

void nscript::parse_func(value_t& result, bool skip)
{
	args_list args;
	parse_args(args);
	parser::state state = _parser.get_state();
	if(_parser.get_token() == parser::end)	throw std::system_error(errc::syntax_error, "'fn'");
	// _varnames contains list of variables to be captured by function
	if(!skip)	_varnames.clear();
	parse<Assignment>(result, true);
	if(!skip)	result = std::make_shared<user_function>(args, _parser.get_content(state, _parser.get_state()), &_context, &_varnames);
}

// Parse "object [(<arguments>)] {<body>}" statement
void nscript::parse_obj(value_t& result, bool skip)
{
	args_list args;
	parse_args(args);
	if(_parser.get_token() == parser::lcurly)	_parser.next();
	parser::state state = _parser.get_state();
	if(_parser.get_token() == parser::end)	throw std::system_error(errc::syntax_error, "'object'");
	// _varnames contains list of variables to be captured by object
	if(!skip)	_varnames.clear();
	parse<Script>(result, true);
	if(!skip)	result = std::make_shared<user_class>(args, _parser.get_content(state, _parser.get_state()), &_context, &_varnames);
	if(_parser.get_token() == parser::rcurly)	_parser.next();
}

#pragma endregion

#pragma region Parser

static std::unordered_map<string_t, parser::token> s_keywords = {
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
	int c, cc;
	while(isspace(c = read()));
	switch(c)	{
		case '\0':	_token = end;break;
		case '+':	cc = peek(); _token = (cc == '+' ? read(), unaryplus  : cc == '=' ? read(), plusset  : plus); break;
		case '-':	cc = peek(); _token = (cc == '-' ? read(), unaryminus : cc == '=' ? read(), minusset : minus); break;
		case '*':	_token = peek() == '=' ? read(), mulset  : multiply;break;
		case '/':	_token = peek() == '=' ? read(), divset  : divide;break;
		case '\\':	_token = peek() == '=' ? read(), idivset : lambda;break;
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
				if(auto p = s_keywords.find(_name); p != s_keywords.end()) _token = p->second; else _token = name;
			}
			break;
	}
	return _token;
}

// Parse integer/double/hexadecimal value from input stream
void parser::read_number(int c)
{
	enum number_stage {nsint, nsdot, nsexp, nspwr, nshex} stage = nsint;
	int base = 10, m = c - '0';
	int e1 = 0, e2 = 0, esign = 1;
	bool overflow = false;

	if(c == '0' && tolower(peek()) == 'x')	{stage = nshex; base = 16; read();}

	while(c = read())	{	
		if(isdigit(c))	{
			int v = c - '0';
			if(stage == nsint || stage == nshex) {
				if(m > (INT_MAX - v) / base)	throw std::system_error(std::make_error_code(std::errc::value_too_large), "number");
				m = m * base + v;
			}
			else if(stage == nsexp)		stage = nspwr;
			else if(stage == nsdot && !overflow)	{
				if(m > (INT_MAX - v) / base)	overflow = true;
				else							m = m * base + v, e1--;
			}
			if(stage == nspwr)		e2 = e2 * 10  + v;
		}	else if(isxdigit(c) && stage == nshex)		{
			int v = 10 + (toupper(c) - 'A');
			if(m > (INT_MAX - v) / base)	throw std::system_error(std::make_error_code(std::errc::value_too_large), "number");
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
	if(stage == nsexp)	throw std::system_error(errc::syntax_error, "number");
	if(stage == nsint || stage == nshex)	_value = double(m);					// integer
	else									_value = m * pow(10., e1+esign*e2);	// floating-point
	_token = parser::value;
}

// Parse quoted string from input stream
void parser::read_string(int quote)	{
	string_t temp;
	for(;;temp += read())	{
		state endpos = _content.find(quote, _pos);
		if(endpos == string_t::npos)	throw std::system_error(errc::missing_character, string_t("'") + (string_t::value_type)quote + "'");
		temp += _content.substr(_pos, endpos-_pos);
		_pos = endpos+1;
		if(peek() != quote)	break;
	}
	_value = temp;
}

// Parse object name from input stream
void parser::read_name(int c)	
{
	if(!isalpha(c) && c != '@' && c != '_')		throw std::system_error(errc::syntax_error, "name");
	_name = c;
	while(isalnum(c = read()) || c == '_')	_name += c;
	back();
}

void parser::check_pair(parser::token token)
{
	if(token == lpar && _token != rpar)			throw std::system_error(errc::missing_character, "')'");
	if(token == lsquare && _token != rsquare)	throw std::system_error(errc::missing_character, "']'");
	if(token == lcurly && _token != rcurly)		throw std::system_error(errc::missing_character, "'}'");
	if(token == lpar || token == lsquare || token == lcurly)	next();
}

#pragma endregion

}