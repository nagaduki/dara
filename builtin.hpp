#pragma once
#include <chrono>
#include <set>

#include "class.hpp"
#include "instance.hpp"
#include "callable.hpp"
#include "error.hpp"
#include "value.hpp"

namespace dara::backend {

class BuiltinClock : public Callable {
   public:
	size_t arity() const override { return 0; }
	std::string to_string() const override { return "<native fn len>"; }
	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter,
	    const std::vector<dara::Value>& arguments) override;
};

class BuiltinLen : public Callable {
   public:
	size_t arity() const override { return 1; }

	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;

	std::string to_string() const override { return "<native fn len>"; }
};

class BuiltinType : public Callable {
   public:
	size_t arity() const override { return 1; }

	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn type>"; }
};

class BuiltinAssert : public Callable {
   public:
	size_t arity() const override { return 2; }
	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn assert>"; }
};

class BuiltinProps : public Callable {
   public:
	size_t arity() const override { return 1; }

	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn props>"; }
};

class BuiltinId : public Callable {
   public:
	size_t arity() const override { return 1; }

	std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn id>"; }
};

class BuiltinIs_A : public Callable {
public:
    size_t arity() const override { return 2; }
    std::expected<dara::Value, InterpreterError> call( Interpreter& interpreter, const std::vector<Value>& arguments) override;
    std::string to_string() const override { return "<native fn is_a>"; }
private:
    bool check_inheritance(std::shared_ptr<dara::runtime::Class> current, std::shared_ptr<dara::runtime::Class> target);
};

class BuiltinIs_Proper : public Callable {
public:
    size_t arity() const override { return 2; }
    std::expected<dara::Value, InterpreterError> call( Interpreter& interpreter, const std::vector<Value>& arguments) override;
    std::string to_string() const override { return "<native fn is_proper>"; }
private:
    bool check_inheritance(std::shared_ptr<dara::runtime::Class> current, std::shared_ptr<dara::runtime::Class> target);
};

class BuiltinRequire : public Callable {
public:
    size_t arity() const override { return 1; }
    std::expected<dara::Value, InterpreterError> call( Interpreter& interpreter, const std::vector<Value>& arguments) override;
    std::string to_string() const override { return "<native fn require>"; }
private:
    bool check_inheritance(std::shared_ptr<dara::runtime::Class> current, std::shared_ptr<dara::runtime::Class> target);

};


}  // namespace dara::backend
