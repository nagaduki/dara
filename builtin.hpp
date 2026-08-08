#pragma once
#include <chrono>

#include "callable.hpp"
#include "error.hpp"
#include "value.hpp"

namespace lox::backend {

class NativeClock : public Callable {
   public:
	size_t arity() const override { return 0; }

	std::expected<lox::Value, InterpreterError> call(
	    Interpreter& interpreter,
	    const std::vector<lox::Value>& arguments) override {
		auto now = std::chrono::system_clock::now().time_since_epoch();
		int seconds =
		    std::chrono::duration_cast<std::chrono::seconds>(now).count();

		return lox::Value{.data = seconds};
	}

	std::string to_string() const override { return "<native fn clock>"; }
};

class NativeLen : public Callable {
   public:
	size_t arity() const override { return 1; }

	std::expected<lox::Value, InterpreterError> call(
	    Interpreter& interpreter,
	    const std::vector<Value>& arguments) override {
		const auto& arg = arguments.at(0);

		if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
			// return lox::Value{.data = arr->size()};
			return lox::Value{.data = static_cast<int>(arr->size())};
		}

		if (const auto* str = std::get_if<std::string>(&arg.data)) {
			return lox::Value{.data = static_cast<int>(str->length())};
		}
	}

	std::string to_string() const override { return "<native fn len>"; }
};

}  // namespace lox::backend
