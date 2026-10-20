#pragma once
#include <chrono>
#include <set>

#include "callable.hpp"
#include "class.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "value.hpp"

namespace dara::backend {

// dispatcher
std::expected<Value, dara::error::InterpreterError> get_string_property(
    Interpreter& interpreter, const String& str,
    const dara::ast::Identifier& name);

// string
class BuiltinStringContains : public Callable {
   public:
	BuiltinStringContains(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_contains>";
	}

   private:
	String str;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinStringLength : public Callable {
   public:
	BuiltinStringLength(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_length>";
	}

   private:
	String str;
};

class BuiltinStringStartsWith : public Callable {
   public:
	BuiltinStringStartsWith(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_start_with>";
	}

   private:
	String str;
};

class BuiltinStringEndsWith : public Callable {
   public:
	BuiltinStringEndsWith(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_starts_with>";
	}

   private:
	String str;
};

// BuiltinStringSubstring in builtinn.hpp
class BuiltinStringSubstring : public Callable {
   public:
	BuiltinStringSubstring(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return -1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_substring>";
	}

   private:
	String str;
};

// BuiltinStringSubstring in builtinn.hpp
class BuiltinStringSplit : public Callable {
   public:
	BuiltinStringSplit(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_split>";
	}

   private:
	String str;
};

class BuiltinStringTrim : public Callable {
   public:
	BuiltinStringTrim(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_trim>";
	}

   private:
	String str;
};

class BuiltinStringToUpper : public Callable {
   public:
	BuiltinStringToUpper(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_toupper>";
	}

   private:
	String str;
};

class BuiltinStringToLower : public Callable {
   public:
	BuiltinStringToLower(String str)
	    : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_tolower>";
	}

   private:
	String str;
};



}
