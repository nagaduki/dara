/* callable.cpp */
#include "callable.hpp"

#include <expected>
#include <memory>
#include <string>
#include <vector>

#include "callable.hpp"
#include "interpreter.hpp"

namespace dara::backend {

Result<dara::Value> Function::call(Interpreter& interpreter,
                                  const std::vector<dara::Value>& arguments) {
	auto local_env = std::make_shared<Environment>(this->closure);

	for (size_t i = 0; i < declaration->parameters.size(); ++i) {
		local_env->define(declaration->parameters[i], arguments[i]);
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
                return std::unexpected(res.error()); //(*)
			}
		}

		interpreter.set_environment(previous_env);

	} catch (const dara::backend::ReturnException& ret) {
		interpreter.set_environment(previous_env);
		return ret.value;
	}

	return dara::Value{std::monostate{}};
}

}  // namespace dara::backend
