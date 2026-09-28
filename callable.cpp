/* callable.cpp */
#include "callable.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "callable.hpp"
#include "interpreter.hpp"

namespace dara::backend {

using namespace dara::ast;

Result<Value> Function::call(Interpreter& interpreter,
                             const std::vector<Value>& arguments) {
	// auto local_env = std::make_shared<Environment>(this->closure);
	auto local_env = std::allocate_shared<Environment>(
	    interpreter.get_allocator(), interpreter.get_allocator(),
	    this->closure);

	for (size_t i = 0; i < declaration->parameters.size(); ++i) {
		local_env->define(declaration->parameters[i].lexeme, arguments[i]);
	}

	std::shared_ptr<Environment> previous_env = interpreter.get_environment();

	try {
		interpreter.set_environment(local_env);

		/* (loop) */
		for (const auto& stmt : declaration->body) {
			auto res = interpreter.exec(stmt.get());
			//
			if (!res) {
				interpreter.set_environment(previous_env);
				return std::unexpected(res.error());  //(*)
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
	auto env = std::allocate_shared<Environment>(interpreter.get_allocator(),
	                                             interpreter.get_allocator(),
	                                             this->closure);
	env->define("this", Value{instance});
	return std::allocate_shared<Function>(interpreter.get_allocator(),
	                                      this->declaration, env);
}

}  // namespace dara::backend
