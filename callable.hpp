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

//class dara::error::InterpreterError;
//class InterpreterError; //(*)

namespace dara::backend {

class Interpreter;

class Callable {
   public:
	virtual ~Callable() = default;
	virtual size_t arity() const = 0;

	virtual std::expected<dara::backend::Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<dara::backend::Value>& argument) = 0;

	virtual std::string to_string() const = 0;
};

class Function : public Callable {
   private:
	const dara::ast::FunctionExpr* declaration;

	std::shared_ptr<Environment> closure;

   public:
	Function(const dara::ast::FunctionExpr* declaration,
	         std::shared_ptr<Environment> closure)
	    : declaration(declaration), closure(closure) {}

	size_t arity() const override { return declaration->parameters.size(); }

    dara::ast::Result<dara::backend::Value> call(Interpreter& interpreter,
	                        const std::vector<dara::backend::Value>& arguments) override;

	std::shared_ptr<Function> bind(Interpreter& interpreter, std::shared_ptr<dara::backend::Instance> instance);
    /*
	std::shared_ptr<Function> bind(
	    std::shared_ptr<dara::backend::Instance> instance) {
		//auto env = std::make_shared<Environment>(this->closure); //(**)
		auto env = std::allocate_shared<Environment>(interpreter.get_allocator(), interpreter.get_allocator(), this->closure); //(****)
		env->define("this", Value{instance});

		return std::make_shared<Function>(this->declaration, env);
	}
    */

	virtual std::string to_string() const { return "<Fn>"; }
};

class LambdaCallable : public Callable {
   private:
	using FunctionType = std::function<dara::ast::Result<dara::backend::Value>(
	    Interpreter&, const std::vector<dara::backend::Value>&)>;
	size_t m_arity;
    FunctionType m_function;

   public: 
    LambdaCallable(size_t arity, FunctionType fn) : m_arity(arity), m_function(std::move(fn)) {}
    size_t arity() const override { return m_arity; }
    dara::ast::Result<dara::backend::Value> call(Interpreter& interpreter, const std::vector<dara::backend::Value>& arguments) override {
        return m_function(interpreter, arguments);
    }
    std::string to_string()const override { return "<native bound method>"; }

};

}  // namespace dara::backend
