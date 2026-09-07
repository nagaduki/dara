/* callable */
#pragma once

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "ast.hpp"
#include "controlflow.hpp"
#include "environment.hpp"
#include "value.hpp"

class InterpreterError;

namespace dara::backend {

class Interpreter;

class Callable {
   public:
	virtual ~Callable() = default;
	virtual size_t arity() const = 0;

	virtual std::expected<dara::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<dara::Value>& argument) = 0;

	virtual std::string to_string() const = 0;
};

class Function : public Callable {
   private:
	const FunctionExpr* declaration;

	std::shared_ptr<Environment> closure;

   public:
	Function(const FunctionExpr* declaration,
	         std::shared_ptr<Environment> closure)
	    : declaration(declaration), closure(closure) {}

	size_t arity() const override { return declaration->parameters.size(); }

	Result<dara::Value> call(Interpreter& interpreter,
	                        const std::vector<dara::Value>& arguments) override;

	std::shared_ptr<Function> bind(
	    std::shared_ptr<dara::runtime::Instance> instance) {
		auto env = std::make_shared<Environment>(this->closure);
		env->define("this", Value{instance});

		return std::make_shared<Function>(this->declaration, env);
	}

	virtual std::string to_string() const { return "<Fn>"; }
};

class LambdaCallable : public Callable {
   private:
	using FunctionType = std::function<Result<dara::Value>(
	    Interpreter&, const std::vector<dara::Value>&)>;
	size_t m_arity;
	FunctionType m_function;

   public: 
    LambdaCallable(size_t arity, FunctionType fn) : m_arity(arity), m_function(std::move(fn)) {}
    size_t arity() const override { return m_arity; }
    Result<dara::Value> call(Interpreter& interpreter, const std::vector<dara::Value>& arguments) override {
        return m_function(interpreter, arguments);
    }
    std::string to_string()const override { return "<native bound method>"; }

};

}  // namespace dara::backend
