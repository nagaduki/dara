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
std::expected<Value, dara::error::InterpreterError> get_array_property(
    Interpreter& interpreter, const std::shared_ptr<Array> array,
    const dara::ast::Identifier& name);


// Array
class BuiltinArrayPush : public Callable {
   public:
	BuiltinArrayPush(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_push>"; }

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayPop : public Callable {
   public:
	BuiltinArrayPop(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_pop>"; }

   private:
	std::shared_ptr<Array> array;
};

class BuiltinArrayContains : public Callable {
   public:
	BuiltinArrayContains(std::shared_ptr<Array> array)
	    : array(std::move(array)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn array_contains>";
	}

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayIndex : public Callable {
   public:
	BuiltinArrayIndex(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_index>"; }

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayInsert : public Callable {
   public:
	BuiltinArrayInsert(std::shared_ptr<Array> array)
	    : array(std::move(array)) {}
	size_t arity() const override { return 2; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn array_insert>";
	}

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayClear : public Callable {
   public:
	BuiltinArrayClear(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_clear>"; }

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayMap : public Callable {
   public:
	BuiltinArrayMap(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_map>"; }

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayFilter : public Callable {
   public:
	BuiltinArrayFilter(std::shared_ptr<Array> array)
	    : array(std::move(array)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn array_filter>";
	}

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayReduce : public Callable {
   public:
	BuiltinArrayReduce(std::shared_ptr<Array> array)
	    : array(std::move(array)) {}
	size_t arity() const override { return 2; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn array_select>";
	}

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayClone : public Callable {
   public:
	BuiltinArrayClone(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_clone>"; }

   private:
	std::shared_ptr<Array> array;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinArrayJoin : public Callable {
   public:
	BuiltinArrayJoin(std::shared_ptr<Array> array) : array(std::move(array)) {}
	size_t arity() const override { return -1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn array_join>"; }

   private:
	std::shared_ptr<Array> array;
};



}
