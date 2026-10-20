// #include <algorithm>
// #include <chrono>
// #include <format>
// #include <memory>
// #include <memory_resource>

//#include "builtin.hpp"
#include "map.hpp"
#include "callable.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "value.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

using String = std::pmr::string;

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace dara::backend {

using namespace dara::ast;

// in builtin_map.cpp
std::expected<Value, dara::error::InterpreterError> BuiltinMapHas::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(
		    dara::error::InterpreterError("has() expects exactly 1 argument"));
	}
	const auto& arg = arguments.at(0);

	// using String = std::pmr::string;
	/*
	  if (!std::holds_alternative<String>(arg.data)) {
	      // if (!std::holds_alternative<std::string>(arg.data)) {
	      return std::unexpected(dara::error::InterpreterError(
	          "first argument to has() must be an string key"));
	  }
	  */
	// using String = std::pmr::string;
	const auto* str_ptr = std::get_if<String>(&arg.data);  //(****)
	if (!str_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "first argument to has() must be a string key"));
	}
	bool found = this->map->contains(*str_ptr);
	//bool found = this->map->contains(&arg.data);
	// bool found = ((*this->map).find(*str_ptr) != (*this->map).end());

	return Value{.data = found};
}

std::expected<Value, dara::error::InterpreterError> BuiltinMapKeys::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(
		    dara::error::InterpreterError("Keys() expects 0 arguments"));
	}

	// auto keys_array =
	// std::allocate_shared<Array>(interpreter.get_allocator(),
	// interpreter.get_allocator());
	auto keys_array = std::allocate_shared<Array>(interpreter.get_allocator());

	for (const auto& [key, value] : *this->map) {
		keys_array->push_back(Value{.data = key});
	}

	return Value{.data = keys_array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinMapValues::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(
		    dara::error::InterpreterError("Values() expects 0 arguments"));
	}

	auto values_array =
	    std::allocate_shared<Array>(interpreter.get_allocator());

	for (const auto& [key, value] : *this->map) {
		values_array->push_back(Value{value});
	}

	return Value{.data = values_array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinMapGet::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 2) {
		return std::unexpected(
		    dara::error::InterpreterError("get() expects exactly 2 arguments"));
	}
	const auto& key = arguments.at(0);
	const auto& default_value = arguments.at(1);

	/*
	if (!std::holds_alternative<String>(key.data)) {
	    // if (!std::holds_alternative<std::string>(arg.data)) {
	    return std::unexpected(dara::error::InterpreterError(
	        "First argument to get() must be an string key"));
	}
	*/

	// using String = std::pmr::string;
	const auto* key_ptr = std::get_if<String>(&key.data);  //(****)
	if (!key_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "first argument to get() must be a string key"));
	}

	//bool found = this->map->contains(*key_ptr);
	auto it = this->map->find(*key_ptr);
	if (it!= this->map->end()) {
		auto it = this->map->find(*key_ptr);
        //return Value{.data = it->first};
        return it->second;
	} else {
	    return default_value;
	}
	// bool found = ((*this->map).find(*str_ptr) != (*this->map).end());

}

std::expected<Value, dara::error::InterpreterError> BuiltinMapClone::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 0) {
		return std::unexpected(dara::error::InterpreterError(
		    "clone() expects exactly 0 argument"));
	}
    /*
	auto filtered_array = std::make_shared<Array>();
	for (const auto& elem : *(this->array)) {
		filtered_array->push_back(elem.clone(interpreter.get_allocator()));
	}
    */
    Value self_val{.data = this->map};

	return self_val.clone(interpreter.get_allocator());
}

std::expected<Value, dara::error::InterpreterError> BuiltinMapClear::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	// const auto& arg = arguments.at(0);

	if (!arguments.empty()) {
		return std::unexpected(
		    dara::error::InterpreterError("clear() take no arguments"));
	}

	this->map->clear();

	return Value{.data = this->map};
}

}  // namespace dara::backend
