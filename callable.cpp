/* callable.cpp */
#include "callable.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>

// #include "callable.hpp"
#include "environment.hpp"
#include "interpreter.hpp"

namespace dara::backend {

using namespace dara::ast;

Result<Value> Function::call(Interpreter& interpreter,
                             const std::vector<Value>& arguments) {
	// auto local_env = std::make_shared<Environment>(this->closure);

    /*
    auto local_env = std::allocate_shared<Environment>(
	    interpreter.get_allocator(), interpreter.get_allocator(),
	    this->closure);
    */
	auto local_env = Environment::create(interpreter.get_allocator(), this->closure);


	for (size_t i = 0; i < declaration->parameters.size(); ++i) {
		// local_env->define(declaration->parameters[i].lexeme, arguments[i]);
		local_env->define(declaration->parameters[i].lexeme,
		                  interpreter.ensure_value_semantics(arguments[i]));
	}

	std::shared_ptr<Environment> previous_env = interpreter.get_environment();

	try {
		interpreter.set_environment(local_env);

		/* (loop) */
		for (const auto& stmt : declaration->body) {
			auto res = interpreter.exec(stmt.get());

			if (!res) {
				interpreter.set_environment(previous_env);
				return std::unexpected(res.error());
			}
		}

		interpreter.set_environment(previous_env);

	} catch (const dara::backend::ReturnException& ret) {
		interpreter.set_environment(previous_env);
		return ret.value;
	}

	return Value{std::monostate{}};
}

std::shared_ptr<Function> Function::bind(
    Interpreter& interpreter,
    std::shared_ptr<dara::backend::Instance> instance) {
	const auto alloc = interpreter.get_allocator();
	auto bound_env = Environment::create(alloc, this->closure);
	bound_env->define("this", Value{std::move(instance)});
	// auto env = std::allocate_shared<Environment>(interpreter.get_allocator(),
	//                                              interpreter.get_allocator(),
	//                                              this->closure);
    auto bound_fn = std::allocate_shared<Function>(alloc, this->declaration, std::move(bound_env));
    bound_fn->is_bound = true;
    return bound_fn;
}

// template <typename T>
// using Result = std::expected<T, dara::error::InterpreterError>;
Result<Value> OverloadedFunction::call(Interpreter& interpreter,
                                       const std::vector<Value>& arguments) {
	size_t arg_count = arguments.size();
	auto it = this->overloads.find(arg_count);
	if (it == this->overloads.end()) {
		return std::unexpected(dara::error::InterpreterError{
		    .message = "No matching overload for '" + this->name + "' taking " +
		               std::to_string(arg_count) + "arguments."});
	}
	return it->second->call(interpreter, arguments);
}

std::shared_ptr<OverloadedFunction> OverloadedFunction::bind(
    Interpreter& interpreter,
    std::shared_ptr<dara::backend::Instance> instance) {
	auto bound = std::allocate_shared<OverloadedFunction>(
	    interpreter.get_allocator(), name);
	for (const auto& [arity, fn] : overloads) {
		bound->add_overload(fn->bind(interpreter, instance));
	}
	return bound;
}

}  // namespace dara::backend
