#pragma once

#include "nscript3.h"

namespace nscript3 {

using std::string;
enum class associativity { left, right };
enum class dereference { none, left, right, both };

struct op_base {
	const parser::token token = parser::token::end;
	const associativity assoc = associativity::left;
	const dereference deref   = dereference::both;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
};

struct op_null : op_base { };

struct op_add : op_base {
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

struct op_sub : op_base {
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

struct op_neg : op_base {
	const parser::token token = parser::token::minus;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return { -y }; }
	template<class X> value_t operator()(X, double y) { return { -y }; }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
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

struct op_mod : op_base	{
	const parser::token token = parser::token::mod;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x % y }; }
	value_t operator()(int x, double y) { return fmod(x, y); }
	value_t operator()(double x, int y) { return fmod(x, y); }
	value_t operator()(double x, double y) { return fmod( x, y); }
};

struct op_pow : op_base {
	const parser::token token = parser::token::pwr;
	using op_base::operator();
	value_t operator()(int x, int y) { return { pow(x, y) }; }
	value_t operator()(int x, double y) { return { pow(x, y) }; }
	value_t operator()(double x, int y) { return { pow(x, y) }; }
	value_t operator()(double x, double y) { return { pow(x, y) }; }
};

// Bitwise
struct op_and : op_base {
	const parser::token token = parser::token::and;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x & y }; }
};

struct op_or : op_base {
	const parser::token token = parser::token::or;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x | y }; }
};

struct op_not : op_base {
	const parser::token token = parser::token::not;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return ~y; }
};

// Comparison
struct comparator {
	int operator() (std::monostate) { return 0; }
	int operator() (int i1, int i2) { return i1 < i2 ? -1 : i1 > i2 ? 1 : 0; }
	int operator() (int i1, double d2) { return i1 < d2 ? -1 : i1 > d2 ? 1 : 0; }
	int operator() (double d1, int i2) { return d1 < i2 ? -1 : d1 > i2 ? 1 : 0; }
	int operator() (double d1, double d2) { return d1 < d2 ? -1 : d1 > d2 ? 1 : 0; }
	int operator() (string s1, string s2) { return s1.compare(s2); }
	template<class X, class Y> int operator()(X x, Y y) { throw nscript_error::type_mismatch; return 0; }
};

struct op_gt : op_base {
	const parser::token token = parser::token::gt;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x,y) > 0; }
};

struct op_lt : op_base {
	const parser::token token = parser::token::lt;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x, y) < 0; }
};

struct op_ge : op_base {
	const parser::token token = parser::token::ge;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x, y) >= 0; }
};

struct op_le : op_base {
	const parser::token token = parser::token::le;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x, y) <= 0; }
};

struct op_eq : op_base {
	const parser::token token = parser::token::equ;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x, y) == 0; }
};

struct op_ne : op_base {
	const parser::token token = parser::token::nequ;
	template<class X, class Y> int operator()(X x, Y y) { return comparator()(x, y) != 0; }
};

struct op_land : op_base {
	const parser::token token = parser::token::land;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x && y }; }
};

struct op_lor : op_base {
	const parser::token token = parser::token:: lor ;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x || y }; }
};

struct op_lnot : op_base {
	const parser::token token = parser::token::lnot;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return !y; }
};

struct op_if : op_base { const parser::token token = parser::token::ifop; };

// Assignments
template <class OP, parser::token TOK>
struct op_xset : op_base {
	const parser::token token = TOK;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class X, class Y> value_t operator()(X x, Y y) { throw nscript_error::missing_lval; return { 0 }; }
	template<class Y> value_t operator()(object_ptr x, Y y) { 
		auto v = std::visit([this, y](auto x) { return OP().operator()(x, y); }, x->get());
		return x->set(v), v; 
	}
};

struct op_ppx : op_xset<op_add, parser::unaryplus>  { 
	const dereference deref = dereference::none;
	template<class X, class Y> value_t operator()(X x, Y y) { return op_xset::operator()(y, 1); }
};
struct op_mmx : op_xset<op_sub, parser::unaryminus> { 
	const dereference deref = dereference::none;
	template<class X, class Y> value_t operator()(X x, Y y) { return op_xset::operator()(y, 1); }
};
struct op_xpp : op_xset<op_add, parser::unaryplus>  { 
	const associativity assoc = associativity::left;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};
struct op_xmm : op_xset<op_sub, parser::unaryminus> { 
	const associativity assoc = associativity::left;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};
struct op_addset : op_xset<op_add, parser::plusset>	{ using op_xset::operator(); };
struct op_subset : op_xset<op_sub, parser::minusset>{ using op_xset::operator(); };
struct op_mulset : op_xset<op_mul, parser::mulset>	{ using op_xset::operator(); };
struct op_divset : op_xset<op_div, parser::divset>	{ using op_xset::operator(); };


struct op_assign : op_base {
	const parser::token token = parser::token::assign;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y)	{ x->set(y);  return {y}; }
	value_t operator()(object_ptr x, object_ptr y)			{ auto v = y->get();  return x->set(v), v; }
	using op_base::operator();
};

struct op_call : op_base	{
	const parser::token token = parser::token::lpar;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y) { return x->call(y); }
	using op_base::operator();
};

struct op_index : op_base {
	const parser::token token = parser::token::lsquare;
	const dereference deref = dereference::right;
	template<class Y> value_t operator()(object_ptr x, Y y) { return x->index(y); }
	using op_base::operator();
};

struct op_statmt : op_base	{
	const parser::token token = parser::token::stmt;
	template<class X, class Y> value_t operator()(X, Y y) { return { y }; }
};

}

template<> struct std::less<nscript3::value_t> {
	bool operator()(const nscript3::value_t &v1, const nscript3::value_t& v2) { return std::visit(nscript3::comparator(), v1, v2) < 0; }
};

