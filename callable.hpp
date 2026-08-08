/* callable */
#pragma once

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "ast.hpp"
#include "environment.hpp"
#include "value.hpp"
#include "controlflow.hpp"

class InterpreterError;

namespace lox::backend {

class Interpreter;

class Callable {
   public:
	virtual ~Callable() = default;
	virtual size_t arity() const = 0;

	virtual std::expected<lox::Value, InterpreterError> call(
	    Interpreter& interpreter, const std::vector<lox::Value>& argument) = 0;

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

	Result<lox::Value> call(Interpreter& interpreter,
	                        const std::vector<lox::Value>& arguments) override;

    std::shared_ptr<Function> bind(std::shared_ptr<lox::runtime::Instance> instance) {
        auto env = std::make_shared<Environment>(this->closure);
        env->define("this", Value{instance});

        return std::make_shared<Function>(this->declaration, env);
    }
    

	virtual std::string to_string() const { return "<Fn>"; }
};

}  // namespace lox::backend
