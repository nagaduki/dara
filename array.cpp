#include "array.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <memory>
#include <memory_resource>

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

std::expected<Value, dara::error::InterpreterError> get_array_property(
    Interpreter& interpreter, const std::shared_ptr<Array> array,
    const dara::ast::Identifier& name) {
	// const auto& alloc = interpreter.get_allocator();
	const auto& alloc = interpreter.get_allocator();

	if (name.lexeme == "size") {
		return Value{.data = static_cast<int>(array->size())};
	}
	if (name.lexeme == "push") {
		std::shared_ptr<dara::backend::Callable> push_method =
		    std::allocate_shared<dara::backend::BuiltinArrayPush>(alloc, array);
		// return Value{.data = std::allocate_shared<
		//           dara::backend::BuiltinNow>(
		//           this->alloc, array)};
		return Value{.data = std::move(push_method)};
	}
	if (name.lexeme == "pop") {
		std::shared_ptr<dara::backend::Callable> pop_method =
		    std::allocate_shared<dara::backend::BuiltinArrayPop>(alloc, array);
		return Value{.data = std::move(pop_method)};
	}
	if (name.lexeme == "index") {
		std::shared_ptr<dara::backend::Callable> index_method =
		    std::allocate_shared<dara::backend::BuiltinArrayIndex>(alloc,
		                                                           array);
		return Value{.data = std::move(index_method)};
	}
	if (name.lexeme == "clear") {
		std::shared_ptr<dara::backend::Callable> clear_method =
		    std::allocate_shared<dara::backend::BuiltinArrayClear>(alloc,
		                                                           array);
		return Value{.data = std::move(clear_method)};
	}

	if (name.lexeme == "insert") {
		std::shared_ptr<dara::backend::Callable> insert_method =
		    std::allocate_shared<dara::backend::BuiltinArrayInsert>(alloc,
		                                                            array);
		return Value{.data = std::move(insert_method)};
	}

	if (name.lexeme == "map") {
		std::shared_ptr<dara::backend::Callable> map_method =
		    std::allocate_shared<dara::backend::BuiltinArrayMap>(alloc, array);
		return Value{.data = std::move(map_method)};
	}

	if (name.lexeme == "filter") {
		std::shared_ptr<dara::backend::Callable> filter_method =
		    std::allocate_shared<dara::backend::BuiltinArrayFilter>(alloc,
		                                                            array);
		return Value{.data = std::move(filter_method)};
	}

	if (name.lexeme == "reduce") {
		std::shared_ptr<dara::backend::Callable> filter_method =
		    std::allocate_shared<dara::backend::BuiltinArrayReduce>(alloc,
		                                                            array);
		return Value{.data = std::move(filter_method)};
	}

	if (name.lexeme == "clone") {
		std::shared_ptr<dara::backend::Callable> clone_method =
		    std::allocate_shared<dara::backend::BuiltinArrayClone>(alloc,
		                                                           array);
		return Value{.data = std::move(clone_method)};
	}

    /*
	if (name.lexeme == "clone") {
		std::shared_ptr<dara::backend::Callable> clone_method =
		    std::allocate_shared<dara::backend::BuiltinArrayClone>(alloc,
		                                                           array);
		return Value{.data = std::move(clone_method)};
	}
    */

	if (name.lexeme == "join") {
		std::shared_ptr<dara::backend::Callable> join_method =
		    std::allocate_shared<dara::backend::BuiltinArrayJoin>(alloc,
		                                                           array);
		return Value{.data = std::move(join_method)};
	}


	return std::unexpected(dara::error::InterpreterError(
	    "Array does not have proterty '" + name.lexeme + "'.", name.span));
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayPush::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);
	// if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
	/*
	if (const auto* arr = std::get_if<Array>(&arg.data)) {
	    return Value{.data = static_cast<int>(arr->size())};
	}
	if (const auto* str = std::get_if<String>(&arg.data)) {
	    return Value{.data = static_cast<int>(str->length())};
	}
	return std::unexpected(
	    dara::error::InterpreterError("Arguments must be an vector"));
	*/
	// this->array->push_back(arguments[0]); // segmentation error
	this->array->push_back(arg);  // segmentation error
	return Value{.data = std::monostate{}};
	/*
	    return std::visit(
	        overloaded{[](const Array& arr) -> Result<Value> {
	                       return Value{static_cast<int>(arr.size())};
	                   },
	                   [](const String& str) -> Result<Value> {
	                       return Value{static_cast<int>(str.length())};
	                   },
	                   [](const int i) -> Result<Value> {
	                       //return Value{static_cast<int>(str.length())};
	                   },
	                   [](auto&&) -> Result<Value> {
	                       return std::unexpected(dara::error::InterpreterError(
	                           "Argument must be an Array or String"));
	                   }},
	        arg.data);
	    */
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayPop::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	// const auto& arg = arguments.at(0);
	//  this->array->push_back(arg); // segmentation error

	if (this->array->empty()) {
		return std::unexpected(
		    dara::error::InterpreterError("Cannot pop from an empty array"));
	}

	auto pop_data = this->array->back();  // segmentation error
	this->array->pop_back();              // segmentation error

	return pop_data;

	/*
	return std::visit(
	    overloaded{[](const std::shared_ptr<Array> arr) -> Result<Value> {
	                   return Value{.data = arr};
	               },
	               [](const String& str) -> Result<Value> {
	                   return Value{.data = str};
	               },
	               [](const int i) -> Result<Value> {
	                   return Value{.data = i};
	                   // return Value{static_cast<int>(str.length())};
	               },
	               [](auto&&) -> Result<Value> {
	                   return std::unexpected(dara::error::InterpreterError(
	                       "Argument must be an Array or String"));
	               }},
	    pop_data.data);
	*/
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayContains::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "contains() expects exactly 1 argument"));
	}

	const Value& target = arg;

	bool found = std::ranges::contains(*(this->array), target);
	//  this->array->push_back(arg); // segmentation error

	return Value{.data = found};

	/*
	return std::visit(
	    overloaded{[](const std::shared_ptr<Array> arr) -> Result<Value> {
	                   return Value{.data = arr};
	               },
	               [](const String& str) -> Result<Value> {
	                   return Value{.data = str};
	               },
	               [](const int i) -> Result<Value> {
	                   return Value{.data = i};
	                   // return Value{static_cast<int>(str.length())};
	               },
	               [](auto&&) -> Result<Value> {
	                   return std::unexpected(dara::error::InterpreterError(
	                       "Argument must be an Array or String"));
	               }},
	    pop_data.data);
	*/
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayIndex::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "index() expects exactly 1 argument"));
	}

	const Value& target = arg;

	auto it = std::ranges::find(*(this->array), target);
	if (it == this->array->end()) {
		return Value{.data = -1};
	}
	int index_pos = static_cast<int>(std::distance(this->array->begin(), it));
	return Value{.data = index_pos};
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayInsert::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "insert() expects exactly 2 argument"));
	}

	const auto& arg_index = arguments.at(0);
	const auto& arg_value = arguments.at(1);

	if (!std::holds_alternative<int>(arg_index.data)) {
		return std::unexpected(dara::error::InterpreterError(
		    "First argument to insert() must be an integer index"));
	}

	int index = std::get<int>(arg_index.data);

	if (index < 0 || index > static_cast<int>(this->array->size())) {
		return std::unexpected(
		    dara::error::InterpreterError("Index out of bounds for inset()"));
	}

	this->array->insert(this->array->begin() + index, arg_value);

	return Value{.data = this->array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayClear::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	// const auto& arg = arguments.at(0);

	if (!arguments.empty()) {
		return std::unexpected(
		    dara::error::InterpreterError("clear() take no arguments"));
	}

	this->array->clear();

	return Value{.data = this->array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayMap::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	// const auto& arg = arguments.at(0);
	if (arguments.size() != 1) {
		return std::unexpected(
		    dara::error::InterpreterError("map() expects exactly 1 argument"));
	}

	auto* callback_ptr = std::get_if<std::shared_ptr<dara::backend::Callable>>(
	    &arguments[0].data);

	if (!callback_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "Argument to map() must be a function"));
	}

	auto mapped_array = std::make_shared<Array>();

	for (const auto& elem : *(this->array)) {
		auto res = (*callback_ptr)->call(interpreter, {elem});
		if (!res) {
			return std::unexpected(res.error());
		}
		mapped_array->push_back(res.value());
	}
	return Value{.data = mapped_array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayFilter::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	// const auto& arg = arguments.at(0);
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "filter() expects exactly 1 argument"));
	}

	auto* callback_ptr = std::get_if<std::shared_ptr<dara::backend::Callable>>(
	    &arguments[0].data);

	if (!callback_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "Argument to filter() must be a function"));
	}

	auto filtered_array = std::make_shared<Array>();

	for (const auto& elem : *(this->array)) {
		auto res = (*callback_ptr)->call(interpreter, {elem});
		if (!res) {
			return std::unexpected(res.error());
		}

		bool is_truthy = false;
		const auto& ret_data = res.value().data;

		if (auto* b = std::get_if<bool>(&ret_data)) {
			is_truthy = *b;
		} else if (std::holds_alternative<std::monostate>(ret_data)) {
			is_truthy = false;
		} else {
			is_truthy = true;
		}
		if (is_truthy) {
			filtered_array->push_back(elem);
		}
	}
	return Value{.data = filtered_array};
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayReduce::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "filter() expects exactly 1 argument"));
	}

	Value accumulator = arguments[0];
	//
	auto* callback_ptr = std::get_if<std::shared_ptr<dara::backend::Callable>>(
	    &arguments[1].data);

	if (!callback_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "Argument to reduce() must be a function"));
	}

	for (const auto& elem : *(this->array)) {
		auto res = (*callback_ptr)->call(interpreter, {accumulator, elem});
		if (!res) {
			return std::unexpected(res.error());
		}

		accumulator = res.value();
	}
	return accumulator;
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayClone::call(
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
	Value self_val{.data = this->array};

	return self_val.clone(interpreter.get_allocator());
}

std::expected<Value, dara::error::InterpreterError> BuiltinArrayJoin::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() > 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "join() expects 0 or 1 argument, but got " +
		    std::to_string(arguments.size()) + "."));
	}

	char char_buf[1];
	std::string_view delimiter = "";

	if (arguments.size() == 1) {
		const auto& arg_data = arguments.at(0).data;
		if (const auto* char_ptr = std::get_if<char>(&arg_data)) {
			char_buf[0] = *char_ptr;
			delimiter = std::string_view(char_buf, 1);
		} else if (const auto* str_ptr = std::get_if<String>(&arg_data)) {
			delimiter = std::string_view(str_ptr->data(), str_ptr->size());
		} else {
			return std::unexpected(dara::error::InterpreterError(
			    "argument to join() must be a sring or char"));
		}
	}

	String result("", interpreter.get_allocator());

	for (size_t i = 0; i < this->array->size(); ++i) {
		if (i > 0 && !delimiter.empty()) {
			result.append(delimiter.data(), delimiter.size());
		}
		const auto& elem_data = (*this->array)[i].data;

		if (const auto* s_ptr = std::get_if<String>(&elem_data)) {
			result.append(s_ptr->data(), s_ptr->size());
		} else if (const auto* c_ptr = std::get_if<char>(&elem_data)) {
			result.push_back(*c_ptr);
		} else {
			return std::unexpected(dara::error::InterpreterError(
			    "join() requires all array elements to be strings or chars"));
		}
	}
	return Value{.data = std::move(result)};
}

}  // namespace dara::backend
