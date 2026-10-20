#pragma once
#include <chrono>
#include <set>

#include "callable.hpp"
#include "class.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "value.hpp"

namespace dara::backend {


// Map
class BuiltinMapHas : public Callable {
   public:
	BuiltinMapHas(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_has>"; }

   private:
	std::shared_ptr<Map> map;
};

class BuiltinMapKeys : public Callable {
   public:
	BuiltinMapKeys(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_keys>"; }

   private:
	std::shared_ptr<Map> map;
};

class BuiltinMapValues : public Callable {
   public:
	BuiltinMapValues(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_values>"; }

   private:
	std::shared_ptr<Map> map;
};

class BuiltinMapGet : public Callable {
   public:
	BuiltinMapGet(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 2; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_get>"; }

   private:
	std::shared_ptr<Map> map;
};

class BuiltinMapClone : public Callable {
   public:
	BuiltinMapClone(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_get>"; }

   private:
	std::shared_ptr<Map> map;
};


class BuiltinMapClear : public Callable {
   public:
	BuiltinMapClear(std::shared_ptr<Map> map) : map(std::move(map)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn map_clear>"; }

   private:
	std::shared_ptr<Map> map;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

}
