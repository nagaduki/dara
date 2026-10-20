#include <algorithm>
#include <chrono>
#include <format>
#include <memory>
#include <memory_resource>

#include "builtin.hpp"
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

std::expected<Value, dara::error::InterpreterError> BuiltinStringContains::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "contains() expects exactly 1 argument"));
	}
	const auto& arg = arguments.at(0);

	// const Value& target = arg;

	const auto* substr_ptr = std::get_if<String>(&arg.data);

	if (!substr_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "argument to contains() must be a string"));
	}

	// using String = std::pmr::string;
	bool found = this->str.contains(*substr_ptr);  // (***)
	//  this->array->push_back(arg); // segmentation error

	return Value{.data = found};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringLength::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 0) {
		return std::unexpected(dara::error::InterpreterError(
		    "length() expects exactly 0 argument"));
	}
	/*
	const auto& arg = arguments.at(0);

	// const Value& target = arg;

	const auto* substr_ptr = std::get_if<String>(&arg.data);

	if (!substr_ptr) {
	    return std::unexpected(dara::error::InterpreterError(
	        "argument to contains() must be a string"));
	}

	// using String = std::pmr::string;
	bool found = this->str.contains(*substr_ptr);  // (***)
	//  this->array->push_back(arg); // segmentation error

	// return Value{.data = found};
	*/
	return Value{static_cast<int>(this->str.length())};
}

std::expected<Value, dara::error::InterpreterError>
BuiltinStringStartsWith::call(Interpreter& interpreter,
                              const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "starts_with() expects exactly 1 argument"));
	}
	const auto& arg_data = arguments.at(0).data;

	if (const auto* substr_ptr = std::get_if<String>(&arg_data)) {
		return Value{.data = this->str.starts_with(*substr_ptr)};
	}

	if (const auto* subchar_ptr = std::get_if<char>(&arg_data)) {
		return Value{.data = this->str.starts_with(*subchar_ptr)};
	}

	return std::unexpected(dara::error::InterpreterError(
	    "argument to starts_with() must be a string or char"));
}

/*
std::expected<Value, dara::error::InterpreterError>
BuiltinStringStartsWith::call(Interpreter& interpreter,
                              const std::vector<Value>& arguments) {
    if (arguments.size() != 1) {
        return std::unexpected(dara::error::InterpreterError(
            "starts_with() expects exactly 1 argument"));
    }
    const auto& arg = arguments.at(0);

    bool found = false;

    const auto* substr_ptr = std::get_if<String>(&arg.data);
    if (!substr_ptr) {
        const auto* subchar_ptr = std::get_if<char>(&arg.data);
        if (!subchar_ptr) {
            return std::unexpected(dara::error::InterpreterError(
                "argument to starts_with() must be a string or char"));
        } else {
            found = this->str.starts_with(*subchar_ptr);  // (***)
        }
    } else {
        found = this->str.starts_with(*substr_ptr);  // (***)
    }
    return Value{.data = found};
}
*/

std::expected<Value, dara::error::InterpreterError> BuiltinStringEndsWith::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "starts_with() expects exactly 1 argument"));
	}
	const auto& arg_data = arguments.at(0).data;

	if (const auto* substr_ptr = std::get_if<String>(&arg_data)) {
		return Value{.data = this->str.ends_with(*substr_ptr)};
	}

	if (const auto* subchar_ptr = std::get_if<char>(&arg_data)) {
		return Value{.data = this->str.ends_with(*subchar_ptr)};
	}

	return std::unexpected(dara::error::InterpreterError(
	    "argument to ends_with() must be a string or char"));
}

std::expected<Value, dara::error::InterpreterError>
BuiltinStringSubstring::call(Interpreter& interpreter,
                             const std::vector<Value>& arguments) {
	if (arguments.size() < 1 || arguments.size() > 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "substring() expects exactly 1 or 2 argument"));
	}
	const auto& start_ptr = std::get_if<int>(&arguments.at(0).data);
	if (!start_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "first argument 'start' must be an integer"));
	}
	int start = *start_ptr;

	size_t length = String::npos;

	if (arguments.size() == 2) {
		const auto* len_ptr = std::get_if<int>(&arguments.at(1).data);
		if (!len_ptr) {
			return std::unexpected(dara::error::InterpreterError(
			    "second argument 'length' must be an integer"));
		}
		int len_val = *len_ptr;

		if (len_val < 0) {
			return std::unexpected(
			    dara::error::InterpreterError("'length' can not be negative"));
		}
		length = static_cast<size_t>(len_val);
	}

	if (start < 0) {
		return std::unexpected(
		    dara::error::InterpreterError("start index can not be negative"));
	}
	size_t str_len = this->str.length();

	if (static_cast<size_t>(start) >= str_len) {
		return Value{.data = String("", interpreter.get_allocator())};
	}

	std::string temp_substr = std::string(this->str).substr(start, length);
	String result(temp_substr.c_str(), temp_substr.length(),
	              interpreter.get_allocator());

	return Value{.data = std::move(result)};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringSplit::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "split() expects exactly 1 argument(delimiter)"));
	}
	auto result_array =
	    std::allocate_shared<Array>(interpreter.get_allocator());
	const auto& arg_data = arguments.at(0).data;

	if (const auto* str_ptr = std::get_if<String>(&arg_data)) {
		const String& delimiter = *str_ptr;

		if (delimiter.empty()) {
			for (char c : this->str) {
				String single_char_str(1, c, interpreter.get_allocator());
				result_array->push_back(
				    Value{.data = std::move(single_char_str)});
			}
			return Value{.data = result_array};
		}
		size_t start = 0;
		size_t end = this->str.find(delimiter);

		while (end != String::npos) {
			//
			String token(this->str.data() + start, end - start,
			             interpreter.get_allocator());
			result_array->push_back(Value{.data = std::move(token)});
			start = end + delimiter.length();
			end = this->str.find(delimiter, start);
		}
		String last_token(this->str.data() + start, this->str.length() - start,
		                  interpreter.get_allocator());
		result_array->push_back(Value{.data = std::move(last_token)});
	} else if (const auto* char_ptr = std::get_if<char>(&arg_data)) {
		char delimiter = *char_ptr;
		size_t start = 0;
		size_t end = this->str.find(delimiter, start);
		while (end != String::npos) {
			//
			String token(this->str.data() + start, end - start,
			             interpreter.get_allocator());
			result_array->push_back(Value{.data = std::move(token)});
			start = end + 1;
			end = this->str.find(delimiter, start);
		}
		String last_token(this->str.data() + start, this->str.length() - start,
		                  interpreter.get_allocator());
		result_array->push_back(Value{.data = std::move(last_token)});

	} else {
		return std::unexpected(dara::error::InterpreterError(
		    "argument to split() must be astring or chat"));
	}
	return Value{.data = result_array};
}

}  // namespace dara::backend
