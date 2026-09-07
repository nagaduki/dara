#include "instance.hpp"

#include <memory>

#include "ast.hpp"
#include "callable.hpp"
#include "class.hpp"

namespace dara::runtime {

// Instance.get
Result<dara::Value> Instance::get(const std::string& name) {
	if (fields.contains(name)) {
		return fields.at(name);
	}

    dara::Value method_val = cls->find_method(name);

	//if (cls->methods.contains(name)) {
	//if (auto method_opt = cls->find_method(name)) {

	if (!std::holds_alternative<std::monostate>(method_val.data)) {
		// user function
		//auto method_val = cls->methods.at(name);
		//auto method_val = method_opt.value();

		// builtin function
		// auto callable =
		//    std::get<std::shared_ptr<dara::backend::Callable>>(method_val.data);

		// user function
		if (auto* callable_ptr =
		        std::get_if<std::shared_ptr<dara::backend::Callable>>(
		            &method_val.data)) {
			auto callable = *callable_ptr;

			if (auto function =
			        std::dynamic_pointer_cast<dara::backend::Function>(
			            *callable_ptr)) {
				auto bound_method = function->bind(shared_from_this());  //(*)

				return Value{std::static_pointer_cast<dara::backend::Callable>(
				    bound_method)};
			}

			// builtin function
			auto receiver = shared_from_this();
			auto bound_closure = [receiver, callable](
			                         dara::backend::Interpreter& interpreter,
			                         const std::vector<dara::Value>& arguments)
			    -> Result<dara::Value> {
				std::vector<dara::Value> args_with_this;
				args_with_this.push_back(dara::Value{.data = receiver});
				for (const auto& arg : arguments) {
					args_with_this.push_back(arg);
				}
				return callable->call(interpreter, args_with_this);
			};
			size_t new_arity =
			    (callable->arity() > 0) ? callable->arity() - 1 : 0; //(***)
			auto bound_method = std::make_shared<dara::backend::LambdaCallable>(
			    new_arity, std::move(bound_closure));
			//return dara::Value{.data = bound_method};
			return dara::Value{std::static_pointer_cast<dara::backend::Callable>(bound_method)};
		}
	}

	return std::unexpected(
	    InterpreterError("undefined property '" + name + "'"));
}

void Instance::set(const std::string& name, Value value) {
	fields[name] = std::move(value);
}

std::vector<std::string> Instance::get_property_names() const {
	std::vector<std::string> names;

	for (const auto& [name, _] : this->fields) {
		names.push_back(name);
	}

    // method
	if (this->cls) {
		for (const auto& [name, _] : this->cls->methods) {
			names.push_back(name + "()");
		}
	}

    /* 
    // static
	if (this->cls) {
		for (const auto& [name, _] : this->cls->static_methods) {
			names.push_back(name + "()");
		}
	}
    */

	return names;
}

std::shared_ptr<Class> Instance::get_class() const { return this->cls; }

}  // namespace dara::runtime
