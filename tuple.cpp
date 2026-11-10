/* tubple.cpp */
#include "tuple.hpp"

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

std::expected<Value, dara::error::InterpreterError> BuiltinTupleSize::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!arguments.empty()) {
		return std::unexpected(dara::error::InterpreterError(
		    "size() expects exactly 0 argument"));
	}

	return Value{.data = static_cast<int>(this->tuple->size())};
}

}
