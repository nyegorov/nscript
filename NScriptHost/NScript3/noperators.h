#pragma once

#include <iomanip>
#include "nscript3.h"

namespace nscript3 {

using std::string;
enum class associativity { left, right, none };
enum class dereference { none, left, right, both };

struct op_base {
	const parser::token token = parser::token::end;
	const associativity assoc = associativity::left;
	const dereference deref   = dereference::both;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::system_error(std::make_error_code(std::errc::operation_not_supported), "op_base"); }
};

struct op_null : op_base { };

#pragma region Conversion

tm date2tm(std::chrono::system_clock::time_point date)
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
		int operator() (std::monostate) { return 0; }
		int operator() (int i) { return i; }
		int operator() (double d) { return (int)(d + 0.5); }
		int operator() (string s) { return std::stoi(s); }
		int operator() (object_ptr o) { throw std::system_error(errc::type_mismatch, "to_int"); }
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
		double operator() (object_ptr o) { throw std::system_error(errc::type_mismatch, "to_double"); }
	};
	return std::visit(to_double_t(), v);
}

tm to_date(const string& s)
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
	mktime(&tm);
	return tm;
}

tm to_date(value_t v)
{
	return to_date(to_string(v));
}

#pragma endregion

#pragma region Mathematical

struct op_add : op_base {
	const parser::token token = parser::token::plus;
	using op_base::operator();
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

#pragma endregion // +, -, *, /

#pragma region Bitwise

struct op_and : op_base {
	const parser::token token = parser::token::and;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x & y }; }
};

struct op_or : op_base {
	const parser::token token = parser::token::or;
	using op_base::operator();
	value_t operator()(int x, int y) { return { x | y }; }
	template<class X> value_t operator()(X x, object_ptr y) { 
		return y->call(x); 
	}
};

struct op_not : op_base {
	const parser::token token = parser::token::not;
	const associativity assoc = associativity::right;
	using op_base::operator();
	template<class X> value_t operator()(X, int y) { return ~y; }
};

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

#pragma endregion		 // >, <, >=, <=, ==, !=, &&, ||, !

#pragma region Assignments

template <class OP, parser::token TOK>
struct op_xset : op_base {
	const parser::token token = TOK;
	const associativity assoc = associativity::right;
	const dereference deref = dereference::right;
	template<class X, class Y> value_t operator()(X x, Y y) { throw std::system_error(errc::missing_lval, "xset"); }
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
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};
struct op_xmm : op_xset<op_sub, parser::unaryminus> { 
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y y) { auto v = *value_t{ x }; op_xset::operator()(x, 1); return v; }
};
struct op_addset : op_xset<op_add, parser::plusset>	{ using op_xset::operator(); };
struct op_subset : op_xset<op_sub, parser::minusset>{ using op_xset::operator(); };
struct op_mulset : op_xset<op_mul, parser::mulset>	{ using op_xset::operator(); };
struct op_divset : op_xset<op_div, parser::divset>	{ using op_xset::operator(); };

#pragma endregion  // ++, --, +=, -=, *=, /=

#pragma region Objects

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

struct op_item : op_base {
	const parser::token token = parser::token::dot;
	const dereference deref = dereference::right;
	value_t operator()(object_ptr x, string_t y) { return x->item(y); }
	using op_base::operator();
};

struct op_new : op_base {
	const parser::token token = parser::token::newobj;
	const dereference deref = dereference::none;
	const associativity assoc = associativity::right;
	template<class X> value_t operator()(X, object_ptr y) { return y->create(); }
	using op_base::operator();
};

struct op_statmt : op_base	{
	const parser::token token = parser::token::stmt;
	//template<class X> value_t operator()(X x, std::monostate) { return { x }; }
	template<class X, class Y> value_t operator()(X x, Y y)	{ return { y }; }
};

#pragma endregion      // ;, =, call, index, item, new

#pragma region Functional

class composer : public object {
	object_ptr		_left;
	object_ptr		_right;
public:
	composer(object_ptr left, object_ptr right) : _left(left), _right(right) {}
	value_t call(value_t params) { return _left->call(_right->call(params)); }
};

struct op_dot : op_base {
	const parser::token token = parser::token::mdot;
	using op_base::operator();
	value_t operator()(object_ptr x, object_ptr y) { return std::make_shared<composer>(x, y); }
};

struct op_head : op_base {
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

struct op_tail : op_base {
	const parser::token token = parser::token::apo;
	const associativity assoc = associativity::none;
	template<class X, class Y> value_t operator()(X x, Y) { return value_t{}; }
	template<class Y> value_t operator()(object_ptr x, Y) { 
		if(auto pa = to_array_if(x); pa)	return pa->empty() ? value_t{} : std::make_shared<v_array>(pa->begin() + 1, pa->end()); 
		throw std::system_error(std::make_error_code(std::errc::invalid_argument), "op_tail");
	}
};

struct op_join : op_base {
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
#pragma endregion   // ·, `, :

}

