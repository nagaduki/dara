#include "instance.hpp"
#include "class.hpp"
#include "callable.hpp"
#include "ast.hpp"
#include <memory>

namespace lox::runtime {
Result<lox::Value> Instance::get(const std::string& name) {
	if (fields.contains(name)) {
		return fields.at(name);
	}

	if (cls->methods.contains(name)) {
		auto method_val = cls->methods.at(name);

		if (auto* callable_ptr =
		        std::get_if<std::shared_ptr<lox::backend::Callable>>(
		            &method_val.data)) {
			if (auto function =
			        std::dynamic_pointer_cast<lox::backend::Function>(
			            *callable_ptr)) {
				auto bound_method = function->bind(shared_from_this()); //(*)

				return Value{std::static_pointer_cast<lox::backend::Callable>(
				    bound_method)};
			}
		}
        
	}

	return std::unexpected(
	    InterpreterError("undefined property '" + name + "'"));
}

void Instance::set(const std::string& name, Value value) {
    fields[name] = std::move(value);
}

}  // namespace lox::runtime
