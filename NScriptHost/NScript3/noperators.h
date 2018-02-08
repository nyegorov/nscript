#pragma once

#include "nscript3.h"

namespace nscript3 {

using std::string;
//enum class associativity { left, right, none };
//enum class dereference { none, left, right, both };

struct op_base {
	const parser::token token = parser::token::end;
	const associativity assoc = associativity::left;
	const dereference deref   = dereference::both;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::system_error(std::make_error_code(std::errc::operation_not_supported), "op_base"); }
};

struct op_null : op_base { };

#pragma region Conversion

tm date2tm(date_t date)
{
	auto t = std::chrono::system_clock::to_time_t(date);
	tm tm = { 0 };
	localtime_s(&tm, &t);
	return tm;
}

std::string to_string(value_t v)
{
	struct print_value {
		string operator() (std::monostate) { return ""; }
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
		string operator() (std::exception_ptr pex) {
			try { std::rethrow_exception(pex); } catch(const std::exception& ex) { return ex.what(); }
			return "unknown exception";
		}
	};

	return std::visit(print_value(), v);
}

int to_int(value_t v)
{
	struct to_int_t {
		int operator() (std::monostate) { return 0; }
		int operator() (int i) { return i; }
		int operator() (double d) { return (int)(d + 0.5); }
		int operator() (string s) { return std::stoi(s); }
		int operator() (date_t dt) { throw std::system_error(errc::type_mismatch, "to_int"); }
		int operator() (object_ptr o) { throw std::system_error(errc::type_mismatch, "to_int"); }
		int operator() (std::exception_ptr) { throw std::system_error(errc::type_mismatch, "to_int"); }
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
		double operator() (date_t dt) { throw std::system_error(errc::type_mismatch, "to_double"); }
		double operator() (object_ptr o) { throw std::system_error(errc::type_mismatch, "to_double"); }
		double operator() (std::exception_ptr) { throw std::system_error(errc::type_mismatch, "to_double"); }
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
			if(stage > year)					throw std::system_error(errc::syntax_error, "to_date");
			stage = date_stage(stage + 1);
		} else if(c == ':') {
			if(stage == day)	date[3] = date[0], date[0] = 1, date[1] = 1, date[2] = 1970, stage = hour;
			if(stage < hour || stage == sec)	throw std::system_error(errc::syntax_error, "to_date");
			stage = date_stage(stage + 1);
		} else if(c == ' ') {
			if(stage != year && stage != hour)	throw std::system_error(errc::syntax_error, "to_date");
			stage = hour;
		} else	break;
	};
	if(date[2] < 100)	date[2] += date[2] < 50 ? 2000 : 1900;
	if(date[0] <= 0 || date[0] > 31)		throw std::system_error(errc::syntax_error, "to_date");
	if(date[1] <= 0 || date[1] > 12)		throw std::system_error(errc::syntax_error, "to_date");
	if(date[2] < 1900 || date[2] > 9999)	throw std::system_error(errc::syntax_error, "to_date");
	if(date[3] < 0 || date[3] > 23)			throw std::system_error(errc::syntax_error, "to_date");
	if(date[4] < 0 || date[4] > 59)			throw std::system_error(errc::syntax_error, "to_date");
	if(date[5] < 0 || date[5] > 59)			throw std::system_error(errc::syntax_error, "to_date");
	tm tm = { 0 };
	tm.tm_isdst = -1;
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
		date_t operator() (std::monostate) { throw std::system_error(errc::type_mismatch, "to_date"); }
		date_t operator() (int) { throw std::system_error(errc::type_mismatch, "to_date"); }
		date_t operator() (double) { throw std::system_error(errc::type_mismatch, "to_date"); }
		date_t operator() (string s) { return to_date(s); }
		date_t operator() (date_t dt) { return dt; }
		date_t operator() (object_ptr) { throw std::system_error(errc::type_mismatch, "to_date"); }
		date_t operator() (std::exception_ptr) { throw std::system_error(errc::type_mismatch, "to_date"); }
	};
	return std::visit(to_date_t(), v);
}

#pragma endregion

#pragma region Mathematical

struct v_add : op_base {
	const parser::token token = parser::token::plus;
	using op_base::operator();
	template<class Y> value_t operator()(object_ptr x, Y y)	{ return std::visit([this, y](auto x) { return operator()(x, y); }, x->get()); }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
	value_t operator()(object_ptr x, object_ptr y)			{ return std::visit([this](auto x, auto y) { return operator()(x, y); }, x->get(), y->get()); }

	value_t operator()(int x, int y)		{ return x + y; }
	value_t operator()(int x, double y)		{ return x + y; }
	value_t operator()(double x, int y)		{ return x + y; }
	value_t operator()(double x, double y)	{ return x + y; }
	value_t operator()(string x, string y)	{ return x + y; }
	value_t operator()(string x, int y)		{ return x + std::to_string(y); }
	value_t operator()(string x, double y)	{ return x + std::to_string(y); }
	value_t operator()(int x, string y)		{ return std::to_string(x) + y; }
	value_t operator()(double x, string y)	{ return std::to_string(x) + y; }
};

struct v_sub : op_base {
	const parser::token token = parser::token::minus;
	using op_base::operator();
	template<class Y> value_t operator()(object_ptr x, Y y) { return std::visit([this, y](auto x) { return operator()(x, y); }, x->get()); }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
	value_t operator()(object_ptr x, object_ptr y)			{ return std::visit([this](auto x, auto y) { return operator()(x, y); }, x->get(), y->get()); }

	value_t operator()(int x, int y)		{ return { x - y }; }
	value_t operator()(int x, double y)		{ return { x - y }; }
	value_t operator()(double x, int y)		{ return { x - y }; }
	value_t operator()(double x, double y)	{ return { x - y }; }
};

struct v_neg : op_base {
	const parser::token token = parser::token::minus;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return { -y }; }
	template<class X> value_t operator()(X, double y) { return { -y }; }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
};

struct v_mul : op_base {
	const parser::token token = parser::token::multiply;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x * y }; }
	value_t operator()(int x, double y) { return { x * y }; }
	value_t operator()(double x, int y) { return { x * y }; }
	value_t operator()(double x, double y) { return { x * y }; }
};

struct v_div : op_base {
	const parser::token token = parser::token::divide;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x / y }; }
	value_t operator()(int x, double y) { return { x / y }; }
	value_t operator()(double x, int y) { return { x / y }; }
	value_t operator()(double x, double y) { return { x / y }; }
};

struct v_mod : op_base	{
	const parser::token token = parser::token::mod;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x % y }; }
	value_t operator()(int x, double y) { return fmod(x, y); }
	value_t operator()(double x, int y) { return fmod(x, y); }
	value_t operator()(double x, double y) { return fmod( x, y); }
};

struct v_pow : op_base {
	const parser::token token = parser::token::pwr;
	using op_base::operator();
	value_t operator()(int x, int y) { return { pow(x, y) }; }
	value_t operator()(int x, double y) { return { pow(x, y) }; }
	value_t operator()(double x, int y) { return { pow(x, y) }; }
	value_t operator()(double x, double y) { return { pow(x, y) }; }
};

op_info op_add{ parser::token::plus,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_add(), x, y); } };
op_info op_sub{ parser::token::minus,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_sub(), x, y); } };
op_info op_mul{ parser::token::multiply,associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_mul(), x, y); } };
op_info op_div{ parser::token::divide,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_div(), x, y); } };
op_info op_mod{ parser::token::mod,		associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_mod(), x, y); } };
op_info op_pow{ parser::token::pwr,		associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_pow(), x, y); } };
op_info op_neg{ parser::token::minus,	associativity::right,dereference::both, [](value_t& x, value_t& y) { return std::visit(v_neg(), x, y); } };

#pragma endregion // +, -, *, /

#pragma region Bitwise

struct v_and : op_base {
	const parser::token token = parser::token::and;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x & y }; }
};

struct v_or : op_base {
	const parser::token token = parser::token::or;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x | y }; }
	template<class X> value_t operator()(X x, object_ptr y) { 
		return y->call(x); 
	}
};

struct v_not : op_base {
	const parser::token token = parser::token::not;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return ~y; }
};

op_info op_and{ parser::token::and,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_and(), x, y); } };
op_info op_or { parser::token::or,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_or(), x, y); } };
op_info op_not{ parser::token::not,	associativity::right,dereference::both, [](value_t& x, value_t& y) { return std::visit(v_not(), x, y); } };

#pragma endregion      // &, |, ~

#pragma region Logical

struct comparator {
	template<class X> int operator() (std::monostate, X) { return 1; }
	template<class Y> int operator() (Y, std::monostate) { return -1; }
	int operator() (std::monostate, std::monostate) { return 0; }
	int operator() (int i1, int i2) { return i1 < i2 ? -1 : i1 > i2 ? 1 : 0; }
	int operator() (int i1, double d2) { return i1 < d2 ? -1 : i1 > d2 ? 1 : 0; }
	int operator() (double d1, int i2) { return d1 < i2 ? -1 : d1 > i2 ? 1 : 0; }
	int operator() (double d1, double d2) { return d1 < d2 ? -1 : d1 > d2 ? 1 : 0; }
	int operator() (string s1, string s2) { return s1.compare(s2); }
	int operator() (object_ptr o1, object_ptr o2) {
		if(auto a1 = to_array_if(o1), a2 = to_array_if(o2); a1 && a2) {
			if(a1->size() != a2->size())	return operator()((int)a1->size(), (int)a2->size());
			auto[p1, p2] = std::mismatch(a1->begin(), a1->end(), a2->begin(), a2->end());
			if(p1 == a1->end() || p2 == a2->end())	return 0;
			return std::visit(*this, *p1, *p2);
		}
		return o1 < o2 ? -1 : o1 > o2 ? 1 : 0;
	}
	template<class X, class Y> int operator()(X x, Y y) { throw std::system_error(errc::type_mismatch, "compare"); }
};

struct v_gt : op_base {
	const parser::token token = parser::token::gt;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x,y) > 0; }
};

struct v_lt : op_base {
	const parser::token token = parser::token::lt;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x, y) < 0; }
};

struct v_ge : op_base {
	const parser::token token = parser::token::ge;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x, y) >= 0; }
};

struct v_le : op_base {
	const parser::token token = parser::token::le;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x, y) <= 0; }
};

struct v_eq : op_base {
	const parser::token token = parser::token::equ;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x, y) == 0; }
};

struct v_ne : op_base {
	const parser::token token = parser::token::nequ;
	template<class X, class Y> value_t operator()(X x, Y y) { return comparator()(x, y) != 0; }
};

struct v_land : op_base {
	const parser::token token = parser::token::land;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x && y }; }
};

struct v_lor : op_base {
	const parser::token token = parser::token:: lor ;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x || y }; }
};

struct v_lnot : op_base {
	const parser::token token = parser::token::lnot;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return !y; }
};

struct v_if : op_base { const parser::token token = parser::token::ifop; };

op_info op_gt{ parser::token::gt,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_gt(), x, y); } };
op_info op_ge{ parser::token::ge,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_ge(), x, y); } };
op_info op_lt{ parser::token::lt,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_lt(), x, y); } };
op_info op_le{ parser::token::le,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_le(), x, y); } };
op_info op_eq{ parser::token::equ,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_eq(), x, y); } };
op_info op_ne{ parser::token::nequ,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_ne(), x, y); } };
op_info op_land{parser::token::land,associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_land(), x, y); } };
op_info op_lor{ parser::token::lor,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_lor(), x, y); } };
op_info op_lnot{parser::token::lnot,associativity::right,dereference::both, [](value_t& x, value_t& y) { return std::visit(v_lnot(), x, y); } };
op_info op_if{ parser::token::ifop,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_if(), x, y); } };

#pragma endregion		 // >, <, >=, <=, ==, !=, &&, ||, !

#pragma region Assignments

template <class OP, parser::token TOK>
struct v_xset : op_base {
	const parser::token token = TOK;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::system_error(errc::missing_lval, "xset"); }
	template<class Y> value_t operator()(object_ptr x, Y y) { 
		auto v = std::visit([this, y](auto x) { return OP().operator()(x, y); }, x->get());
		return x->set(v), v; 
	}
};

struct v_ppx : v_xset<v_add, parser::unaryplus>  { 
	const dereference deref = dereference::none;
	template<class X, class Y> value_t operator()(X x, Y y) { return v_xset::operator()(y, 1); }
};
struct v_mmx : v_xset<v_sub, parser::unaryminus> { 
	const dereference deref = dereference::none;
	template<class X, class Y> value_t operator()(X x, Y y) { return v_xset::operator()(y, 1); }
};
struct v_xpp : v_xset<v_add, parser::unaryplus>  { 
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; v_xset::operator()(x, 1); return v; }
};
struct v_xmm : v_xset<v_sub, parser::unaryminus> { 
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; v_xset::operator()(x, 1); return v; }
};
struct v_addset : v_xset<v_add, parser::plusset>	{ using v_xset::operator(); };
struct v_subset : v_xset<v_sub, parser::minusset>	{ using v_xset::operator(); };
struct v_mulset : v_xset<v_mul, parser::mulset>	{ using v_xset::operator(); };
struct v_divset : v_xset<v_div, parser::divset>	{ using v_xset::operator(); };

op_info op_ppx{ parser::token::unaryplus,	associativity::right, dereference::none,  [](value_t& x, value_t& y) { return std::visit(v_ppx(), x, y); } };
op_info op_mmx{ parser::token::unaryminus,	associativity::right, dereference::none,  [](value_t& x, value_t& y) { return std::visit(v_mmx(), x, y); } };
op_info op_xpp{ parser::token::unaryplus,	associativity::none,  dereference::right, [](value_t& x, value_t& y) { return std::visit(v_xpp(), x, y); } };
op_info op_xmm{ parser::token::unaryminus,	associativity::none,  dereference::right, [](value_t& x, value_t& y) { return std::visit(v_xmm(), x, y); } };
op_info op_addset{ parser::token::plusset,	associativity::right, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_addset(), x, y); } };
op_info op_subset{ parser::token::minusset,	associativity::right, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_subset(), x, y); } };
op_info op_mulset{ parser::token::mulset,	associativity::right, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_mulset(), x, y); } };
op_info op_divset{ parser::token::divset,	associativity::right, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_divset(), x, y); } };

#pragma endregion  // ++, --, +=, -=, *=, /=

#pragma region Objects

struct v_assign : op_base {
	const parser::token token = parser::token::assign;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y)	{ x->set(y);  return {y}; }
	value_t operator()(object_ptr x, object_ptr y)			{ auto v = y->get();  return x->set(v), v; }
	using op_base::operator();
};

struct v_call : op_base	{
	const parser::token token = parser::token::lpar;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y) { return x->call(y); }
	using op_base::operator();
};

struct v_index : op_base {
	const parser::token token = parser::token::lsquare;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y) { return x->index(y); }
	using op_base::operator();
};

struct v_item : op_base {
	const parser::token token = parser::token::dot;
	const dereference deref = dereference::right;
	value_t operator()(object_ptr x, string_t y) { return x->item(y); }
	using op_base::operator();
};

struct v_new : op_base {
	const parser::token token = parser::token::newobj;
	const dereference deref = dereference::none;
	const associativity assoc = associativity::right;
	template<class X> value_t operator()(X, object_ptr y) { return y->create(); }
	using op_base::operator();
};

struct v_statmt : op_base	{
	const parser::token token = parser::token::stmt;
	//template<class X> value_t operator()(X x, std::monostate) { return { x }; }
	template<class X, class Y> value_t operator()(X x, Y y)	{ return { y }; }
};

op_info op_assign{	parser::token::assign,	associativity::right,dereference::right, [](value_t& x, value_t& y) { return std::visit(v_assign(), x, y); } };
op_info op_call{	parser::token::lpar,	associativity::left, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_call(), x, y); } };
op_info op_index{	parser::token::lsquare,	associativity::left, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_index(), x, y); } };
op_info op_item{	parser::token::dot,		associativity::left, dereference::right, [](value_t& x, value_t& y) { return std::visit(v_item(), x, y); } };
op_info op_new{		parser::token::newobj,	associativity::right,dereference::none,  [](value_t& x, value_t& y) { return std::visit(v_new(), x, y); } };
op_info op_stmt{	parser::token::stmt,	associativity::left, dereference::both,  [](value_t& x, value_t& y) { return std::visit(v_statmt(), x, y); } };

#pragma endregion      // ;, =, call, index, item, new

#pragma region Functional

class composer : public object {
	object_ptr		_left;
	object_ptr		_right;
public:
	composer(object_ptr left, object_ptr right) : _left(left), _right(right) {}
	value_t call(value_t params) { return _left->call(_right->call(params)); }
};

struct v_dot : op_base {
	const parser::token token = parser::token::mdot;
	using op_base::operator();
	value_t operator()(object_ptr x, object_ptr y) { return std::make_shared<composer>(x, y); }
};

struct v_head : op_base {
	const parser::token token = parser::token::apo;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class X, class Y> value_t operator()(X, Y y) { return y; }
	template<class X> value_t operator()(X, object_ptr y) { 
		if(auto pa = std::dynamic_pointer_cast<v_array>(y); pa) 
			return pa->items().empty() ? value_t{} : pa->items().front(); 
		throw std::system_error(std::make_error_code(std::errc::invalid_argument), "op_head");
	}
};

struct v_tail : op_base {
	const parser::token token = parser::token::apo;
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y) { return value_t{}; }
	template<class Y> value_t operator()(object_ptr x, Y) { 
		if(auto pa = to_array_if(x); pa)	return pa->empty() ? value_t{} : std::make_shared<v_array>(pa->begin() + 1, pa->end()); 
		throw std::system_error(std::make_error_code(std::errc::invalid_argument), "op_tail");
	}
};

struct v_join : op_base {
	const parser::token token = parser::token::colon;
	template<class X, class Y> value_t operator()(X x, Y y) { 
		if(is_empty(x))	return { y };
		if(is_empty(y))	return { x };
		if(auto ys = to_array_if({ y }); ys)
			return ys->insert(ys->begin(), x), y;
		else
			return std::make_shared<v_array>(std::initializer_list<value_t>{x, y});
	}
	template<class Y> value_t operator()(object_ptr x, Y y) {
		if(is_empty(y))	return { x };
		if(auto xs = to_array_if(x); xs) {
			if(auto ys = to_array_if({ y }); ys)
				std::copy(ys->begin(), ys->end(), std::back_inserter(*xs));
			else
				xs->emplace_back(y);
			return x;
		}	else {
			if(auto ys = to_array_if({ y }); ys)
				return ys->insert(ys->begin(), x), y;
			else
				return std::make_shared<v_array>(std::initializer_list<value_t>{x, y});
		}
	}
};

op_info op_dot{  parser::token::mdot,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_dot(), x, y); } };
op_info op_head{ parser::token::apo,	associativity::right,dereference::right,[](value_t& x, value_t& y) { return std::visit(v_head(), x, y); } };
op_info op_tail{ parser::token::apo,	associativity::none, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_tail(), x, y); } };
op_info op_join{ parser::token::colon,	associativity::left, dereference::both, [](value_t& x, value_t& y) { return std::visit(v_join(), x, y); } };

#pragma endregion   // ·, `, :

}

