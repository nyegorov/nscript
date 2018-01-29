#pragma once

#include "nscript3.h"

namespace nscript3 {

using std::string;
enum class associativity { left, right };

template<class T> struct op_base {
	const parser::token token = parser::token::end;
	const associativity assoc = associativity::left;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	template<class Y> value_t operator()(object_ptr x, Y y) { return /*std::visit([this, y](auto&& x) { return static_cast<T*>(this)->operator()(x, y); }, x->get())*/{ 1 }; }
	template<class X> value_t operator()(X x, object_ptr y) { return /*std::visit([this, x](auto&& y) { return static_cast<T*>(this)->operator()(x, y); }, y->get())*/{ 1 }; }
	value_t operator()(object_ptr x, object_ptr y) { return /*std::visit([this, x](auto&& y) { return static_cast<T*>(this)->operator()(x, y); }, y->get())*/{ 3 }; }
	//template<class X> value_t operator()(X x, object_ptr y) { return std::visit(*this, value_t{ x }, y->get()); }
};

struct op_null : op_base<op_null> { };

struct op_add : op_base<op_add> {
	const parser::token token = parser::token::plus;
	template<class X, class Y> value_t operator()(X x, Y y)	{ throw std::errc::operation_not_supported; return { 0 }; }
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

struct op_sub : op_base<op_sub> {
	const parser::token token = parser::token::minus;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	template<class Y> value_t operator()(object_ptr x, Y y) { return std::visit([this, y](auto x) { return operator()(x, y); }, x->get()); }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
	value_t operator()(object_ptr x, object_ptr y)			{ return std::visit([this](auto x, auto y) { return operator()(x, y); }, x->get(), y->get()); }

	value_t operator()(int x, int y)		{ return { x - y }; }
	value_t operator()(int x, double y)		{ return { x - y }; }
	value_t operator()(double x, int y)		{ return { x - y }; }
	value_t operator()(double x, double y)	{ return { x - y }; }
};

struct op_neg : op_base<op_neg> {
	const parser::token token = parser::token::minus;
	const associativity assoc = associativity::right;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	template<class X> value_t operator()(X, int y) { return { -y }; }
	template<class X> value_t operator()(X, double y) { return { -y }; }
	template<class X> value_t operator()(X x, object_ptr y) { return std::visit([this, x](auto y) { return operator()(x, y); }, y->get()); }
};

struct op_mul : op_base<op_mul> {
	const parser::token token = parser::token::multiply;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	value_t operator()(int x, int y) { return { x * y }; }
	value_t operator()(int x, double y) { return { x * y }; }
	value_t operator()(double x, int y) { return { x * y }; }
	value_t operator()(double x, double y) { return { x * y }; }
};

struct op_div : op_base<op_div> {
	const parser::token token = parser::token::divide;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	value_t operator()(int x, int y) { return { x / y }; }
	value_t operator()(int x, double y) { return { x / y }; }
	value_t operator()(double x, int y) { return { x / y }; }
	value_t operator()(double x, double y) { return { x / y }; }
};

struct op_pow : op_base<op_pow> {
	const parser::token token = parser::token::pwr;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
	value_t operator()(int x, int y) { return { pow(x, y) }; }
	value_t operator()(int x, double y) { return { pow(x, y) }; }
	value_t operator()(double x, int y) { return { pow(x, y) }; }
	value_t operator()(double x, double y) { return { pow(x, y) }; }
};

template <class OP, parser::token TOK>
struct op_xset : op_base<op_xset<OP,TOK>> {
	const parser::token token = TOK;
	template<class X, class Y> value_t operator()(X x, Y y) { throw nscript_error::missing_lval; return { 0 }; }
	template<class Y> value_t operator()(object_ptr x, Y y) { 
		auto v = std::visit([this, y](auto x) { return OP().operator()(x, y); }, x->get());
		return x->set(v), v; 
	}
};

struct op_ppx : op_xset<op_add, parser::unaryplus>  { 
	const associativity assoc = associativity::right;
	template<class X, class Y> value_t operator()(X x, Y y) { return op_xset::operator()(y, 1); }
};
struct op_mmx : op_xset<op_sub, parser::unaryminus> { 
	const associativity assoc = associativity::right;
	template<class X, class Y> value_t operator()(X x, Y y) { return op_xset::operator()(y, 1); }
};
struct op_xpp : op_xset<op_add, parser::unaryplus>  { 
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};
struct op_xmm : op_xset<op_sub, parser::unaryminus> { 
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};

struct op_assign : op_base<op_assign> {
	const parser::token token = parser::token::assign;
	const associativity assoc = associativity::right;
	template<class Y> value_t operator()(object_ptr x, Y y)	{ x->set(y);  return {y}; }
	value_t operator()(object_ptr x, object_ptr y)			{ auto v = y->get();  return x->set(v), v; }
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::errc::operation_not_supported; return { 0 }; }
};

struct op_statmt : op_base<op_statmt> {
	const parser::token token = parser::token::stmt;
	template<class X, class Y> value_t operator()(X, Y y) { return { y }; }
};;

}