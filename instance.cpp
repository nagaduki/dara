/* instance.cpp */
#include "instance.hpp"

#include <memory>

#include "ast.hpp"
#include "callable.hpp"
#include "class.hpp"
#include "error.hpp"

namespace dara::backend {

using namespace dara::ast;

// get in Instance.cpp //
Result<Value> Instance::get(Interpreter& interpreter, const std::string& name) {
	// if (this->fields.contains(name)) {
	// if (this->fields.contains(name)) {
	//	return fields.at(name);
	//}

	/*
	if (this->cls && this->cls->has_field(name)) {
	    return std::unexpected(dara::error::InterpreterError(
	        "fields '" + name + "' on class '" + this->cls->name +
	        "' is not initialized"));
	}
	*/
	// __get__bind_if_callable__
	if (auto it = this->fields.find(name); it != this->fields.end()) {
		// if (this->cls && this->cls->has_method(name)) {
		if (auto* callable_ptr =
		        std::get_if<std::shared_ptr<dara::backend::Callable>>(
		            &it->second.data)) {
			auto callable = *callable_ptr;
			// __get__user_function__
			if (auto function =
			        std::dynamic_pointer_cast<dara::backend::Function>(
			            *callable_ptr)) {

					if (function->get_is_bound()) {
						return it->second;
					}

				auto bound_method =
				    function->bind(interpreter, shared_from_this());

				return Value{std::static_pointer_cast<dara::backend::Callable>(
				    bound_method)};
			}
			// __get__builtin_function__
			auto receiver = shared_from_this();
			auto bound_closure =
			    [receiver, callable](
			        dara::backend::Interpreter& interpreter,
			        const std::vector<Value>& arguments) -> Result<Value> {
				std::vector<Value> args_with_this;
				args_with_this.reserve(arguments.size() + 1);  //(***)
				args_with_this.push_back(Value{.data = receiver});
				for (const auto& arg : arguments) {
					args_with_this.push_back(arg);
				}
				return callable->call(interpreter, args_with_this);
			};
			size_t raw_arity = callable->arity();
			size_t new_arity =
			    (raw_arity > 0 && raw_arity != static_cast<size_t>(-1))
			        ? raw_arity - 1
			        : 0;
			auto bound_method = std::make_shared<dara::backend::LambdaCallable>(
			    new_arity, std::move(bound_closure));
			// return Value{.data = bound_method};
			return Value{std::static_pointer_cast<dara::backend::Callable>(
			    bound_method)};
		}
		//}
		return it->second;
	}

	// Value method_val = this->cls->find_method(name);
	//  __get__find_member__
	if (this->cls) {
		if (auto it = this->cls->members.find(name);
		    it != this->cls->members.end()) {
			// __get__uninitialize_field
			if (std::holds_alternative<std::monostate>(it->second.data)) {
				return std::unexpected(dara::error::InterpreterError(
				    "field '" + name + "' on class '" + this->cls->name +
				    "' is not initialized"));
			}
			// __get__user_function__
			if (auto* callable_ptr =
			        std::get_if<std::shared_ptr<dara::backend::Callable>>(
			            //&method_val.data)) {
			            &it->second.data)) {
				auto callable = *callable_ptr;

				if (auto function =
				        std::dynamic_pointer_cast<dara::backend::Function>(
				            *callable_ptr)) {
					// __get__check_bound_fn__
					//if (function->get_is_bound()) {
					//	return it->second;
					//}
					auto bound_method =
					    function->bind(interpreter, shared_from_this());  //(*)

					return Value{
					    std::static_pointer_cast<dara::backend::Callable>(
					        bound_method)};
				}

				// __get__builtin_function__
				auto receiver = shared_from_this();
				auto bound_closure =
				    [receiver, callable](
				        dara::backend::Interpreter& interpreter,
				        const std::vector<Value>& arguments) -> Result<Value> {
					std::vector<Value> args_with_this;
					args_with_this.reserve(arguments.size() + 1);  //(***)
					args_with_this.push_back(Value{.data = receiver});
					for (const auto& arg : arguments) {
						args_with_this.push_back(arg);
					}
					return callable->call(interpreter, args_with_this);
				};
				size_t raw_arity = callable->arity();
				size_t new_arity =
				    (raw_arity > 0) ? callable->arity() - 1 : 0;  //(***)
				auto bound_method =
				    std::make_shared<dara::backend::LambdaCallable>(
				        new_arity, std::move(bound_closure));
				// return Value{.data = bound_method};
				return Value{std::static_pointer_cast<dara::backend::Callable>(
				    bound_method)};
			}
			return it->second;
		}
	}
	return std::unexpected(
	    dara::error::InterpreterError("undefined property '" + name + "'"));
}
/*
// Value method_val = this->cls->find_method(name);
Value method_val = this->cls->find_method(name);

// if (cls->methods.contains(name)) {
// if (auto method_opt = cls->find_method(name)) {

if (!std::holds_alternative<std::monostate>(method_val.data)) {
    // user function
    // auto method_val = cls->methods.at(name);
    // auto method_val = method_opt.value();

    // builtin function
    // auto callable =
    //    std::get<std::shared_ptr<dara::backend::Callable>>(method_val.data);

    // user function
    if (auto* callable_ptr =
            std::get_if<std::shared_ptr<dara::backend::Callable>>(
                &method_val.data)) {
        auto callable = *callable_ptr;

        if (auto function = std::dynamic_pointer_cast<dara::backend::Function>(
                *callable_ptr)) {
            auto bound_method =
                function->bind(interpreter, shared_from_this());  //(*)

            return Value{std::static_pointer_cast<dara::backend::Callable>(
                bound_method)};
        }

        // builtin function
        auto receiver = shared_from_this();
        auto bound_closure =
            [receiver, callable](
                dara::backend::Interpreter& interpreter,
                const std::vector<Value>& arguments) -> Result<Value> {
            std::vector<Value> args_with_this;
            args_with_this.push_back(Value{.data = receiver});
            for (const auto& arg : arguments) {
                args_with_this.push_back(arg);
            }
            return callable->call(interpreter, args_with_this);
        };
        size_t new_arity =
            (callable->arity() > 0) ? callable->arity() - 1 : 0;  //(***)
        auto bound_method = std::make_shared<dara::backend::LambdaCallable>(
            new_arity, std::move(bound_closure));
        // return Value{.data = bound_method};
        return Value{
            std::static_pointer_cast<dara::backend::Callable>(bound_method)};
    }
}

return std::unexpected(
    dara::error::InterpreterError("undefined property '" + name + "'"));
}

*/

void Instance::set(const std::string& name, Value value) {
	fields[name] = std::move(value);
}

std::vector<std::string> Instance::get_property_names() const {
	std::vector<std::string> names;

	for (const auto& [name, _] : this->fields) {
		if (!this->cls || !this->cls->has_method(name)) {
			names.push_back(name);
		}
	}

	// __set__find_method_and_uninitiaized_field_from_memeber__
	if (this->cls) {
		// for (const auto& [name, _] : this->cls->members) {
		for (const auto& [name, value] : this->cls->members) {
			if (std::holds_alternative<std::monostate>(value.data)) {
				if (!this->fields.contains(name)) {
					// names.push_back(name + "()");
					names.push_back(name);
				}
			} else {
				names.push_back(name + "()");
			}
		}
	}
	return names;
	/*
	// static
	if (this->cls) {
	    for (const auto& [name, _] : this->cls->static_methods) {
	        names.push_back(name + "()");
	    }
	}
	*/
}

std::shared_ptr<Class> Instance::get_class() const { return this->cls; }

}  // namespace dara::backend
