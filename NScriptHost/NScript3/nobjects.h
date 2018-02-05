#pragma once

#include "nscript3.h"

namespace nscript3 {

class object;

// Class that represents script variables
class variable : public object {
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
class narray : public object {
	std::vector<value_t>	_items;
public:
	narray() {}
	template<class InputIt> narray(InputIt first, InputIt last) : _items(first, last) {}
	narray(value_t val) : _items{ val } {}
	value_t get() {
		if(_items.size() == 1) return _items.front();
		return shared_from_this();
	}
	value_t index(value_t index) {
		if(auto pi = std::get_if<int>(&index))	return std::make_shared<indexer>(std::static_pointer_cast<narray>(shared_from_this()), *pi);
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
	std::vector<value_t>& items() { return _items; }

protected:
	class indexer : public object {
		std::shared_ptr<narray>	_data;
		int						_index;
	public:
		indexer(std::shared_ptr<narray> arr, int index) : _index(index), _data(arr) {};
		value_t get() { return _data->items()[_index]; }
		void set(value_t value) { _data->items()[_index] = value; }
		value_t call(value_t params)	{ return std::visit(op_call(), _data->items()[_index], params); }
		value_t item(value_t item)		{ return std::visit(op_item(), _data->items()[_index], item); }
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
		} else if(is_empty(params) && _count <= 0)	return _func({});
		else if(_count < 0 || _count == 1)			return _func({ params });
		else										throw nscript_error::bad_param_count;
	}
protected:
	const int			_count;
	FN					_func;
};
template<class FN> object_ptr make_fn(int count, FN fn) { return std::make_shared<builtin_function<FN>>(count, fn); }

// User-defined functions

void process_args(const args_list& args, const value_t& params, NScript& script) {
	if(args.size() == 0) {
		script.add("@", params);
	} else {
		auto a = to_array(params);
		if(args.size() != a->items().size())	throw nscript_error::bad_param_count;
		for(int i = (int)args.size() - 1; i >= 0; i--)	script.add(args[i], a->items()[i]);
	}
}

class user_function	: public object {
	const args_list		_args;
	const string_t		_body;
	const context		_context;
public:
	user_function(const args_list& args, string_view body, const context *pcontext = nullptr, const context::var_names* pcaptures = nullptr) 
		: _args(args), _body(body), _context(pcontext, pcaptures)	{}
	value_t call(value_t params) {
		NScript script(_body, &_context);
		process_args(_args, params, script);
		return script.eval({});
	}
};

// User-defined classes
class user_class : public object {
	const args_list		_args;
	const string_t		_body;
	const context		_context;
	value_t				_params;
public:
	user_class(const args_list& args, string_view body, const context *pcontext = nullptr, const context::var_names* pcaptures = nullptr)
		: _args(args), _body(body), _context(pcontext, pcaptures) {}
	value_t create() const			{ return std::make_shared<instance>(_body, &_context, _args, _params); }
	value_t call(value_t params)	{ _params = params; return shared_from_this(); }

	class instance : public object {
		NScript				_script;
	public:
		instance(string_view body, const context *pcontext, const args_list& args, value_t params) : _script(body, pcontext) {
			process_args(args, params, _script);
			_script.eval({});
		}
		value_t item(value_t item)	{ return _script.eval(std::get<string_t>(item)); }
	};
};

}