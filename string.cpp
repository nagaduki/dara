#include "string.hpp"

#include <utf8.h>

#include <algorithm>
#include <chrono>
#include <expected>
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

std::expected<Value, dara::error::InterpreterError> get_string_property(
    Interpreter& interpreter, const String& str,
    const dara::ast::Identifier& name) {
	const auto& alloc = interpreter.get_allocator();
	if (name.lexeme == "contains") {
		std::shared_ptr<dara::backend::Callable> contains_method =
		    std::allocate_shared<dara::backend::BuiltinStringContains>(alloc,
		                                                               str);
		return Value{.data = std::move(contains_method)};
	}

	if (name.lexeme == "length") {
		std::shared_ptr<dara::backend::Callable> length_method =
		    std::allocate_shared<dara::backend::BuiltinStringLength>(alloc,
		                                                             str);
		return Value{.data = std::move(length_method)};
	}
	if (name.lexeme == "starts_with") {
		std::shared_ptr<dara::backend::Callable> startswith_method =
		    std::allocate_shared<dara::backend::BuiltinStringStartsWith>(alloc,
		                                                                 str);
		return Value{.data = std::move(startswith_method)};
	}
	if (name.lexeme == "ends_with") {
		std::shared_ptr<dara::backend::Callable> endswith_method =
		    std::allocate_shared<dara::backend::BuiltinStringEndsWith>(alloc,
		                                                               str);
		return Value{.data = std::move(endswith_method)};
	}
	if (name.lexeme == "substring") {
		std::shared_ptr<dara::backend::Callable> substring_method =
		    std::allocate_shared<dara::backend::BuiltinStringSubstring>(alloc,
		                                                                str);
		return Value{.data = std::move(substring_method)};
	}
	if (name.lexeme == "split") {
		std::shared_ptr<dara::backend::Callable> split_method =
		    std::allocate_shared<dara::backend::BuiltinStringSplit>(alloc, str);
		return Value{.data = std::move(split_method)};
	}
	if (name.lexeme == "chars") {
		std::shared_ptr<dara::backend::Callable> chars_method =
		    std::allocate_shared<dara::backend::BuiltinStringChars>(alloc, str);
		return Value{.data = std::move(chars_method)};
	}

	if (name.lexeme == "trim") {
		std::shared_ptr<dara::backend::Callable> split_method =
		    std::allocate_shared<dara::backend::BuiltinStringTrim>(alloc, str);
		return Value{.data = std::move(split_method)};
	}
	if (name.lexeme == "to_upper") {
		std::shared_ptr<dara::backend::Callable> upper_method =
		    std::allocate_shared<dara::backend::BuiltinStringToUpper>(alloc,
		                                                              str);
		return Value{.data = std::move(upper_method)};
	}
	if (name.lexeme == "to_lower") {
		std::shared_ptr<dara::backend::Callable> lower_method =
		    std::allocate_shared<dara::backend::BuiltinStringToLower>(alloc,
		                                                              str);
		return Value{.data = std::move(lower_method)};
	}
	if (name.lexeme == "to_halfwidth") {
		std::shared_ptr<dara::backend::Callable> halfwidth_method =
		    std::allocate_shared<dara::backend::BuiltinStringToHalfwidth>(alloc,
		                                                                  str);
		return Value{.data = std::move(halfwidth_method)};
	}
	if (name.lexeme == "to_fullwidth") {
		std::shared_ptr<dara::backend::Callable> fullwidth_method =
		    std::allocate_shared<dara::backend::BuiltinStringToFullwidth>(alloc,
		                                                                  str);
		return Value{.data = std::move(fullwidth_method)};
	}
	if (name.lexeme == "replace") {
		std::shared_ptr<dara::backend::Callable> fullwidth_method =
		    std::allocate_shared<dara::backend::BuiltinStringReplace>(alloc,
		                                                              str);
		return Value{.data = std::move(fullwidth_method)};
	}
	if (name.lexeme == "index_of") {
		std::shared_ptr<dara::backend::Callable> fullwidth_method =
		    std::allocate_shared<dara::backend::BuiltinStringIndexOf>(alloc,
		                                                              str);
		return Value{.data = std::move(fullwidth_method)};
	}
	if (name.lexeme == "replace_regex") {
		std::shared_ptr<dara::backend::Callable> fullwidth_method =
		    std::allocate_shared<dara::backend::BuiltinStringReplaceRegex>(
		        alloc, str);
		return Value{.data = std::move(fullwidth_method)};
	}
	if (name.lexeme == "matches") {
		std::shared_ptr<dara::backend::Callable> match_method =
		    std::allocate_shared<dara::backend::BuiltinStringMatches>(alloc,
		                                                              str);
		return Value{.data = std::move(match_method)};
	}
	if (name.lexeme == "split_regex") {
		std::shared_ptr<dara::backend::Callable> match_method =
		    std::allocate_shared<dara::backend::BuiltinStringSplitRegex>(alloc,
		                                                                 str);
		return Value{.data = std::move(match_method)};
	}
	if (name.lexeme == "match_all") {
		std::shared_ptr<dara::backend::Callable> match_method =
		    std::allocate_shared<dara::backend::BuiltinStringMatchAll>(alloc,
		                                                                 str);
		return Value{.data = std::move(match_method)};
	}

	return std::unexpected(dara::error::InterpreterError(
	    "string does not have property '" + name.lexeme + "'.", name.span));
}

// 1020 char
/*
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

    // else

    return Value{.data = found};
}
*/
std::expected<Value, dara::error::InterpreterError> BuiltinStringContains::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "contains() expects exactly 1 argument"));
	}

	const auto& arg_data = arguments.at(0).data;
	bool found = false;

	if (const auto* str_ptr = std::get_if<String>(&arg_data)) {
		// if (!str_ptr) {
		//	return std::unexpected(dara::error::InterpreterError(
		//	    "argument to contains() must be a string"));
		// }

		// using String = std::pmr::string;
		found = this->str.contains(*str_ptr);  // (***)
		//  this->array->push_back(arg); // segmentation error

	} else if (const auto* char_ptr = std::get_if<char>(&arg_data)) {
		// char delimiter = *char_ptr;
		found = this->str.contains(*char_ptr);  // (***)

	} else {
		return std::unexpected(dara::error::InterpreterError(
		    "argument to contains() must be astring or chat"));
	}
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
	// return Value{static_cast<int>(this->str.length())};
	int length =
	    static_cast<int>(utf8::distance(this->str.begin(), this->str.end()));
	return Value{.data = length};
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

/*
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
*/

/*
std::expected<Value, dara::error::InterpreterError>
BuiltinStringSubstring::call(Interpreter& interpreter,
                             const std::vector<Value>& arguments) {
    if (arguments.size() < 1 || arguments.size() > 2) {
        return std::unexpected(dara::error::InterpreterError(
            "substring() expects exactly 1 or 2 argument"));
    }
    const auto* start_ptr = std::get_if<int>(&arguments.at(0).data);
    const auto* length_ptr = std::get_if<int>(&arguments.at(1).data);
    if (!start_ptr || !length_ptr) {
        return std::unexpected(dara::error::InterpreterError(
            "substiring() arguments must be integers"));
    }

    int start_idx = *start_ptr;
    int sub_length = *length_ptr;
    int total_length =
        static_cast<int>(utf8::distance(this->str.begin(), this->str.end()));

    if (start_idx < 0 || sub_length < 0 ||
        start_idx + sub_length > total_length) {
        return std::unexpected(
            dara::error::InterpreterError("substring() index out of bounds"));
    }

    auto it_start = this->str.begin();
    utf8::advance(it_start, start_idx, this->str.end());

    auto it_end = it_start;
    utf8::advance(it_end, sub_length, this->str.end());

    String result(&(*it_start), std::distance(it_start, it_end),
interpreter.get_allocator());

    return Value{.data = std::move(result)};
}
*/
std::expected<Value, dara::error::InterpreterError>
BuiltinStringSubstring::call(Interpreter& interpreter,
                             const std::vector<Value>& arguments) {
	if (arguments.size() != 1 && arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "substring() expects exactly 1 or 2 argument"));
	}
	const auto* start_ptr = std::get_if<int>(&arguments.at(0).data);
	if (!start_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "first argument 'start' must be an integer"));
	}

	int start_idx = *start_ptr;
	int total_length =
	    static_cast<int>(utf8::distance(this->str.begin(), this->str.end()));
	if (start_idx < 0 || start_idx > total_length) {
		return std::unexpected(
		    dara::error::InterpreterError("substring() index out of bounds"));
	}
	auto it_start = this->str.begin();
	utf8::advance(it_start, start_idx, this->str.end());

	auto it_end = it_start;

	if (arguments.size() == 2) {
		const auto* length_ptr = std::get_if<int>(&arguments.at(1).data);
		if (!length_ptr) {
			return std::unexpected(dara::error::InterpreterError(
			    "substring() length must be an integer"));
		}
		int sub_length = *length_ptr;
		if (sub_length < 0 || start_idx + sub_length > total_length) {
			return std::unexpected(dara::error::InterpreterError(
			    "substring() length out of bounds"));
		}

		it_end = it_start;
		utf8::advance(it_end, sub_length, this->str.end());
	}

	String result(it_start, it_end, interpreter.get_allocator());
	return Value{.data = std::move(result)};
}

/*
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
*/
std::expected<Value, dara::error::InterpreterError> BuiltinStringSplit::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "split() expects exactly 1 argument(delimiter)"));
	}
	const auto& arg_data = arguments.at(0).data;
	// char char_buf[1];
	// std::string_view delimiter;

	const auto* str_ptr = std::get_if<String>(&arguments.at(0).data);
	if (!str_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "argument to split() must be a string"));
	}
	// std::string_view delimiter(str_ptr->data(), str_ptr->size());
	std::string_view delimiter = *str_ptr;
	Array result(interpreter.get_allocator());

	if (delimiter.empty()) {
		auto it = this->str.begin();
		auto end = this->str.end();

		while (it != end) {
			auto char_start = it;
			uint32_t cp = utf8::next(it, end);

			if (cp >= 0xFF66 && cp <= 0xFF9D && it != end) {
				uint32_t next_cp = utf8::peek_next(it, end);
				if (next_cp == 0xFF9E || next_cp == 0xFF9F) {
					utf8::next(it, end);
				}
			}

			result.push_back(Value{
			    .data = String(char_start, it, interpreter.get_allocator())});
		}

		std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
		    interpreter.get_allocator(), std::move(result));
		return Value{.data = std::move(array_ptr)};
	}

	// std::shared_ptr<Array> array_ptr = std::move(array_ptr);
	// std::string_view target(this->str.data(), this->str.size());
	std::string_view target = this->str;
	size_t start = 0;
	size_t end = target.find(delimiter);

	while (end != std::string_view::npos) {
		std::string_view token = target.substr(start, end - start);
		result.push_back(Value{.data = String(token.data(), token.size(),
		                                      interpreter.get_allocator())});
		start = end + delimiter.size();
		end = target.find(delimiter, start);
	}

	std::string_view last_token = target.substr(start);
	result.push_back(Value{.data = String(last_token.data(), last_token.size(),
	                                      interpreter.get_allocator())});
	// return Value{.data = result_array};

	std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
	    interpreter.get_allocator(), std::move(result));

	return Value{.data = std::move(array_ptr)};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringChars::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 0) {
		return std::unexpected(dara::error::InterpreterError(
		    "chars() expects exactly 0 argument"));
	}

	Array result(interpreter.get_allocator());
	result.reserve(this->str.size());

	auto it = this->str.begin();
	auto end = this->str.end();

	while (it != end) {
		uint32_t cp = utf8::next(it, end);

		if (cp >= 0x80) {
			return std::unexpected(dara::error::InterpreterError(
			    "chars() only suppors ASCII characters. Use split(\"\") for "
			    "UTF-8 sring"));
		}
		result.push_back(Value{.data = static_cast<char>(cp)});
	}
	std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
	    interpreter.get_allocator(), std::move(result));
	return Value{.data = std::move(array_ptr)};
}

/*
std::expected<Value, dara::error::InterpreterError> BuiltinStringTrim::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
    if (!arguments.empty()) {
        return std::unexpected(dara::error::InterpreterError(
            "trim() expects exactly 0 arguments"));
    }

    const char* whitespace = " \t\n\r\f\v";

    size_t start = this->str.find_first_not_of(whitespace);

    if (start == String::npos) {
        return Value{.data = String("", interpreter.get_allocator())};
    }

    size_t end = this->str.find_last_not_of(whitespace);

    size_t trim_length = end - start + 1;

    String trimmed_str(this->str.data() + start, trim_length,
                       interpreter.get_allocator());

    return Value{.data = std::move(trimmed_str)};
}
*/

/*
std::expected<Value, dara::error::InterpreterError> BuiltinStringTrim::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
    if (!arguments.empty()) {
        return std::unexpected(dara::error::InterpreterError(
            "trim() expects exactly 0 arguments"));
    }

    std::string_view sv(this->str.data(), this->str.size());

    constexpr std::string_view whitespace = " \t\n\r\f\v";

    size_t first = sv.find_first_not_of(whitespace);

    if (first == std::string_view::npos) {
        return Value{.data = String("", interpreter.get_allocator())};
    }

    size_t last = sv.find_last_not_of(whitespace);
    std::string_view trimmed = sv.substr(first, last - first + 1);

    return Value{.data = String(trimmed.data(), trimmed.size(),
                                interpreter.get_allocator())};
}
*/
std::expected<Value, dara::error::InterpreterError> BuiltinStringTrim::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "trim() expects exactly 0 arguments"));
	}

	auto is_space_cp = [](uint32_t cp) {
		return cp == ' ' || cp == '\t' || cp == '\n' || cp == 'r' ||
		       cp == '\f' || cp == '\v' || cp == 0x300;
	};

	auto it_start = this->str.begin();
	auto it_end = this->str.end();

	while (it_start != it_end) {
		auto next_it = it_start;
		uint32_t cp = utf8::next(next_it, it_end);
		if (!is_space_cp(cp)) {
			break;
		}
		it_start = next_it;
	}

	while (it_end != it_start) {
		auto prev_it = it_end;
		uint32_t cp = utf8::prior(prev_it, it_start);
		if (!is_space_cp(cp)) {
			break;
		}
		it_end = prev_it;
	}

	String result(it_start, it_end, interpreter.get_allocator());
	return Value{.data = std::move(result)};
}

/*
std::expected<Value, dara::error::InterpreterError>
BuiltinStringToUpper::call( Interpreter& interpreter, const
std::vector<Value>& arguments) { if (!arguments.empty()) { return
std::unexpected(dara::error::InterpreterError( "trim() expects exactly 0
arguments"));
    }

    std::string s(this->str.c_str());
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    String upper_str(s.c_str(), interpreter.get_allocator());

    return Value{.data = std::move(upper_str)};
}
*/

std::expected<Value, dara::error::InterpreterError> BuiltinStringToUpper::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "to_upper() expects exactly 0 arguments"));
	}

	std::string s(this->str.c_str());
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
		return (c < 0x80) ? static_cast<char>(std::toupper(c))
		                  : static_cast<char>(c);
	});

	String upper_str(s.c_str(), interpreter.get_allocator());

	return Value{.data = std::move(upper_str)};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringToLower::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "to_lower expects exactly 0 arguments"));
	}

	std::string s(this->str.c_str());
	std::transform(s.begin(), s.end(), s.begin(),
	               //[](unsigned char c) { return std::tolower(c); });
	               [](unsigned char c) {
		               return (c < 0x80) ? static_cast<char>(std::tolower(c))
		                                 : static_cast<char>(c);
	               });

	String lower_str(s.c_str(), interpreter.get_allocator());

	return Value{.data = std::move(lower_str)};
}

std::expected<Value, dara::error::InterpreterError>
BuiltinStringToHalfwidth::call(Interpreter& interpreter,
                               const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "to_Halfwidth expects exactly 0 arguments"));
	}

	String result("", interpreter.get_allocator());
	result.reserve(this->str.size() * 2);

	auto it = this->str.begin();
	auto end = this->str.end();
	auto out = std::back_inserter(result);

	while (it != end) {
		uint32_t cp = utf8::next(it, end);
		if (cp >= 0xFF01 && cp <= 0xFF5E) {
			result.push_back(static_cast<char>(cp - 0xFEE0));
		} else if (cp == 0x3000) {
			result.push_back(' ');
		} else if (cp >= 0x30A1 && cp <= 0x30F4) {
			const auto& pair = FULL_TO_HALF_KANA[cp - 0x30A1];
			if (pair.base != 0) {
				utf8::append(pair.base, out);
				if (pair.mark != 0) {
					utf8::append(pair.mark, out);
				}
			} else {
				utf8::append(cp, out);
			}
		} else if (cp == 0x3002) {
			utf8::append(0xFF61, out);
		} else if (cp == 0x300C) {
			utf8::append(0xFF62, out);
		} else if (cp == 0x300D) {
			utf8::append(0xFF63, out);
		} else if (cp == 0x3001) {
			utf8::append(0xFF64, out);
		} else if (cp == 0x30FB) {
			utf8::append(0xFF65, out);
		} else if (cp == 0x30FC) {
			utf8::append(0xFF70, out);
		} else if (cp == 0x309B) {
			utf8::append(0xFF9E, out);
		} else if (cp == 0x309C) {
			utf8::append(0xFF6F, out);
		} else {
			utf8::append(cp, out);
		}
	}
	return Value{.data = std::move(result)};
}
std::expected<Value, dara::error::InterpreterError>
BuiltinStringToFullwidth::call(Interpreter& interpreter,
                               const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "to_Halfwidth expects exactly 0 arguments"));
	}

	String result("", interpreter.get_allocator());
	result.reserve(this->str.size() * 2);

	auto it = this->str.begin();
	auto end = this->str.end();
	auto out = std::back_inserter(result);

	while (it != end) {
		uint32_t cp = utf8::next(it, end);
		if (cp >= 0x0021 && cp <= 0x007E) {
			utf8::append(cp + 0xFEE0, out);
		} else if (cp == 0x0020) {
			utf8::append(0x3000, out);
		} else if (cp >= 0xFf61 && cp <= 0xFF9f) {
			const auto& entry = HALF_TO_FULL_KANA[cp - 0xFF61];
			uint16_t mapped = entry.normal;

			if (it != end) {
				uint32_t next_cp = utf8::peek_next(it, end);
				if (next_cp == 0xFF9E && entry.dakuten != 0) {
					mapped = entry.dakuten;
					utf8::next(it, end);
				} else if (next_cp == 0xFF9F && entry.handakuten != 0) {
					mapped = entry.handakuten;
					utf8::next(it, end);
				}
			}
			utf8::append(mapped, out);
		} else {
			utf8::append(cp, out);
		}
	}
	return Value{.data = std::move(result)};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringReplace::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "replace() expects exactly 2 argument"));
	}

	const auto* from_ptr = std::get_if<String>(&arguments.at(0).data);
	const auto* to_ptr = std::get_if<String>(&arguments.at(1).data);
	if (!from_ptr || !to_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "arguments to replace() must be strings"));
	}

	// std::string_view target(this->str.data(), this->str.size());
	// std::string_view from(from_ptr->data(), from_ptr->size());
	// std::string_view to(to_ptr->data(), to_ptr->size());

	std::string_view target = this->str;
	std::string_view from = *from_ptr;
	std::string_view to = *to_ptr;
	if (from.empty()) {
		return Value{.data = this->str};
	}

	size_t pos = target.find(from);

	if (pos == std::string_view::npos) {
		return Value{.data = this->str};
	}

	String result("", interpreter.get_allocator());
	result.reserve(this->str.size());

	size_t start = 0;
	while (pos != std::string_view::npos) {
		result.append(target.data() + start, pos - start);
		result.append(to.data(), to.size());
		start = pos + from.size();
		pos = target.find(from, start);
	}
	// std::string_view tail = target.substr(start);
	std::string_view tail = target.substr(start);
	result.append(tail.data(), tail.size());

	return Value{.data = std::move(result)};
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringIndexOf::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "index_of() expects exactly 1 argument"));
	}
	const auto* sub_ptr = std::get_if<String>(&arguments.at(0).data);
	if (!sub_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "argument to index_of() must be a string"));
	}
	std::string_view target(this->str.data(), this->str.size());
	std::string_view sub(sub_ptr->data(), sub_ptr->size());

	size_t byte_pos = target.find(sub);
	if (byte_pos == std::string_view::npos) {
		return Value{.data = -1};
	}

	auto it_start = this->str.begin();
	auto it_found = this->str.begin() + static_cast<std::ptrdiff_t>(byte_pos);
	int char_index = static_cast<int>(utf8::distance(it_start, it_found));

	return Value{.data = char_index};
}

std::expected<Value, dara::error::InterpreterError>
BuiltinStringReplaceRegex::call(Interpreter& interpreter,
                                const std::vector<Value>& arguments) {
	if (arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "replace_regex() expects exactly 2 argument"));
	}

	const auto* pattern_ptr = std::get_if<String>(&arguments.at(0).data);
	const auto* target_ptr = std::get_if<String>(&arguments.at(1).data);

	if (!pattern_ptr || !target_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "arguments to replace_regex() must be strings"));
	}

	try {
		srell::u8cregex re(pattern_ptr->data(), pattern_ptr->size());
		String result("", interpreter.get_allocator());
		result.reserve(this->str.size());

		srell::regex_replace(std::back_inserter(result), this->str.data(),
		                     this->str.data() + this->str.size(), re,
		                     target_ptr->c_str());

		return Value{.data = std::move(result)};
	} catch (const srell::regex_error& e) {
		return std::unexpected(dara::error::InterpreterError(
		    std::string("regex error in replace_regex(): ") + e.what()));
	}
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringMatches::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "matches() expects exactly 1 argument"));
	}
	const auto* pattern_ptr = std::get_if<String>(&arguments.at(0).data);
	if (!pattern_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "arguments to matches() must be strings"));
	}

	try {
		srell::u8cregex re(pattern_ptr->data(), pattern_ptr->size());
		bool is_matched = srell::regex_search(
		    this->str.data(), this->str.data() + this->str.size(), re);
		return Value{.data = is_matched};
	} catch (const srell::regex_error& e) {
		return std::unexpected(dara::error::InterpreterError(
		    std::string("regex error in matches(): ") + e.what()));
	}
}

std::expected<Value, dara::error::InterpreterError>
BuiltinStringSplitRegex::call(Interpreter& interpreter,
                              const std::vector<Value>& arguments) {
	if (arguments.size() != 1) {
		return std::unexpected(dara::error::InterpreterError(
		    "split_regex() expects exactly 1 argument"));
	}
	const auto* pattern_ptr = std::get_if<String>(&arguments.at(0).data);
	if (!pattern_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "arguments to split_regex() must be strings"));
	}

	Array result(interpreter.get_allocator());

	// pattern is empty
	if (pattern_ptr->empty()) {
		auto it = this->str.begin();
		auto end = this->str.end();
		while (it != end) {
			auto char_start = it;
			uint32_t cp = utf8::next(it, end);
			if (cp >= 0xFF66 && cp <= 0xFF9D && it != end) {
				uint32_t next_cp = utf8::peek_next(it, end);
				if (next_cp == 0xFF9E || next_cp == 0xFF9F) {
					utf8::next(it, end);
				}
			}
			result.push_back(Value{
			    .data = String(char_start, it, interpreter.get_allocator())});
		}
		std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
		    interpreter.get_allocator(), std::move(result));
		return Value{.data = std::move(array_ptr)};
	}

	// pattern is not empty
	try {
		srell::u8cregex re(pattern_ptr->data(), pattern_ptr->size());
		const char* first = this->str.data();
		const char* last = first + this->str.size();
		const char* last_pos = first;

		srell::u8ccregex_iterator it(first, last, re);
		srell::u8ccregex_iterator end;

		for (; it != end; ++it) {
			const srell::u8ccmatch& m = *it;

			if (m[0].first == m[0].second && m[0].first == last_pos) {
				continue;
			}

			result.push_back(
			    Value{.data = String(last_pos, m[0].first,
			                         interpreter.get_allocator())});
			last_pos = m[0].second;
		}
		result.push_back(
		    Value{.data = String(last_pos, last, interpreter.get_allocator())});
		std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
		    interpreter.get_allocator(), std::move(result));
		return Value{.data = std::move(array_ptr)};
	} catch (const srell::regex_error& e) {
		return std::unexpected(dara::error::InterpreterError(
		    std::string("regex error in matches(): ") + e.what()));
	}
}

std::expected<Value, dara::error::InterpreterError> BuiltinStringMatchAll::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (arguments.size() != 1 && arguments.size() != 2) {
		return std::unexpected(dara::error::InterpreterError(
		    "Expected 1 or 2 arguments for match_all(), but got " +
		    std::to_string(arguments.size())));
	}

	const auto* pattern_ptr = std::get_if<String>(&arguments.at(0).data);
	if (!pattern_ptr) {
		return std::unexpected(dara::error::InterpreterError(
		    "first argument to match_all() mus be a string"));
	}

	int group_idx = 0;
	if (arguments.size() == 2) {
		const auto* idx_ptr = std::get_if<int>(&arguments.at(1).data);
		if (!idx_ptr || *idx_ptr < 0) {
			return std::unexpected(dara::error::InterpreterError(
			    "second argument to match_all() must be a non-gegative "
			    "integer"));
		}
		group_idx = *idx_ptr;
	}

	try {
		srell::u8cregex re(pattern_ptr->data(), pattern_ptr->size());
		if (static_cast<size_t>(group_idx) > re.mark_count()) {
			return std::unexpected(dara::error::InterpreterError(
			    "capture group index " + std::to_string(group_idx) +
			    " is out of bounds for pattern"));
		}
		Array result(interpreter.get_allocator());

		const char* first = this->str.data();
		const char* last = first + this->str.size();

		srell::u8ccregex_iterator it(first, last, re);
		srell::u8ccregex_iterator end;

		for (; it != end; ++it) {
			const srell::u8ccmatch& m = *it;
			if (m[group_idx].matched) {
				result.push_back(Value{
				    .data = String(m[group_idx].first, m[group_idx].second,
				                   interpreter.get_allocator())});
			}
		}
		std::shared_ptr<Array> array_ptr = std::allocate_shared<Array>(
		    interpreter.get_allocator(), std::move(result));
		return Value{.data = std::move(array_ptr)};

	} catch (const srell::regex_error& e) {
		return std::unexpected(dara::error::InterpreterError(
		    std::string("regex error in matche_all(): ") + e.what()));
	}
}

}  // namespace dara::backend
