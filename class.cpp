/* class.cpp */
#include "class.hpp"

#include "callable.hpp"
#include "error.hpp"
#include "instance.hpp"  //  ここで Instance の完全な定義を読み込む
#include "interpreter.hpp"

namespace dara::backend {

using namespace dara::ast;

size_t Class::arity() const { return static_cast<size_t>(-1); };

/*
size_t Class::arity() const {
    Value method_val = find_method(this->name);  //(**)
    // if (auto method_val = const_cast<Class*>(this)->find_method(this->name))
    // {
    if (!std::holds_alternative<std::monostate>(method_val.data)) {
        if (auto* callable_ptr =
                std::get_if<std::shared_ptr<dara::backend::Callable>>(
                    &method_val.data)) {
            return (*callable_ptr)->arity();
        }
    }
    return 0;
}
*/

// java like class
bool Class::has_field(const std::string& name) const {
	if (this->fields.contains(name)) {
		return true;
	}
	if (this->super && this->super->has_field(name)) {
		return true;
	}
	for (const auto& mixin : this->mixins) {
		if (mixin && mixin->has_field(name)) {
			return true;
		}
	}
	return false;
}

// Result<void> Class::init_default_fields(
Result<void> Class::init_fields(
    Interpreter& interpreter, const std::shared_ptr<Instance>& instance) const {
	if (this->super) {
		// auto res = this->super->init_default_fields(interpreter, instance);
		auto res = this->super->init_fields(interpreter, instance);
		if (!res) {
			return res;
		}
	}
	// 1027
	for (const auto& mixin : this->mixins) {
		if (mixin) {
			// auto res = mixin->init_default_fields(interpreter, instance);
			auto res = mixin->init_fields(interpreter, instance);
			if (!res) {
				return res;
			}
		}
	}
	for (const auto& [field_name, init_expr] : this->field_initializers) {
		auto val_res = interpreter.visit_expr(init_expr);
		if (!val_res) {
			return std::unexpected(val_res.error());
		}
		instance->set(field_name,
		              interpreter.ensure_value_semantics(val_res.value()));
	}
	return {};
}

/*
class ClassConstructor : public dara::backend::Callable {
private:
std::shared_ptr<Class> cls;

explicit ClassConstructor(std::shared_ptr<Class> cls)
    : cls(std::move(cls)) {}

size_t arity() const override {
    if (cls->methods.contains("initialize")) {
        auto method_val = cls->methods.at("initialize");
        if (auto* callable_ptr =
                std::get_if<std::shared_ptr<dara::backend::Callable>>(
                    &method_val.data)) {
            return (*callable_ptr)->arity();
        }
    }
    return 0;
}

Result<Value> call(dara::backend::Interpreter& interpreter,
                        const std::vector<Value>& arguments) override {
    auto instance = std::make_shared<Instance>(cls);

    if (cls->methods.contains("initialize")) {
        auto init_res = instance->get("initialize");
        if (!init_res) {
            return std::unexpected(init_res.error());
        }
        if (auto* callable_ptr =
                std::get_if<std::shared_ptr<dara::backend::Callable>>(
                    &init_res.value().data)) {
            auto call_res = (*callable_ptr)->call(interpreter, arguments);
            if (!call_res) {
                return std::unexpected(call_res.error());
            }
        }
    } else if (!arguments.empty()) {
        return std::unexpected(
            dara::error::InterpreterError("Expected 0 arguments but got " +
                             std::to_string(arguments.size())));
    }
    return Value{instance};
}

std::string to_string() const override {
    return "<constructor for " + cls->name + ">";
}
};
*/

// overloaded Constructor
Result<Value> Class::call(dara::backend::Interpreter& interpreter,
                          const std::vector<Value>& arguments) {
	// auto instance = std::make_shared<Instance>(shared_from_this());
	auto instance = std::allocate_shared<Instance>(interpreter.get_allocator(),
	                                               shared_from_this());
	//Value method_val = find_method(this->name);

	auto init_fields_res = this->init_fields(interpreter, instance);
	if (!init_fields_res) {
		return std::unexpected(init_fields_res.error());
	}
	if (this->constructors.empty()) {
		if (!arguments.empty()) {
			return std::unexpected(dara::error::InterpreterError(
			    "Expected 0 arguments for class '" + this->name +
			    "', but got " + std::to_string(arguments.size())));
		}
		return Value{instance};
	}

	/*
	// if (this->find_method(this->name)) {
	if (!std::holds_alternative<std::monostate>(method_val.data)) {
	    auto init_res = instance->get(interpreter, this->name);
	    if (!init_res) {
	        return std::unexpected(init_res.error());
	    }
	    if (auto* callable_ptr =
	            std::get_if<std::shared_ptr<dara::backend::Callable>>(
	                &init_res.value().data)) {
	        auto call_res = (*callable_ptr)->call(interpreter, arguments);
	        if (!call_res) {
	            return std::unexpected(call_res.error());
	        }
	    }
	} else if (!arguments.empty()) {
	    return std::unexpected(
	        dara::error::InterpreterError("Expected 0 arguments but got " +
	                                      std::to_string(arguments.size())));
	}
	*/
	auto ctor = this->constructors.find(arguments.size());
	if (!ctor) {
		return std::unexpected(dara::error::InterpreterError(
		    "no matching constructor for class '" + this->name + "' taking " +
		    std::to_string(arguments.size()) + " arguments"));
	}
	auto bound_ctor = ctor->bind(interpreter, instance);
	auto call_res = bound_ctor->call(interpreter, arguments);
	if (!call_res) {
		return std::unexpected(call_res.error());
	}

	return Value{instance};
}

/* before overload
Result<Value> Class::call(dara::backend::Interpreter& interpreter,
                          const std::vector<Value>& arguments) {
    // auto instance = std::make_shared<Instance>(shared_from_this());
    auto instance = std::allocate_shared<Instance>(interpreter.get_allocator(),
                                                   shared_from_this());
    Value method_val = find_method(this->name);  //(**)

    // if (this->find_method(this->name)) {
    if (!std::holds_alternative<std::monostate>(method_val.data)) {
        auto init_res = instance->get(interpreter, this->name);
        if (!init_res) {
            return std::unexpected(init_res.error());
        }
        if (auto* callable_ptr =
                std::get_if<std::shared_ptr<dara::backend::Callable>>(
                    &init_res.value().data)) {
            auto call_res = (*callable_ptr)->call(interpreter, arguments);
            if (!call_res) {
                return std::unexpected(call_res.error());
            }
        }
    } else if (!arguments.empty()) {
        return std::unexpected(
            dara::error::InterpreterError("Expected 0 arguments but got " +
                                          std::to_string(arguments.size())));
    }
    return Value{instance};
}
*/
/*
Result<Value> Class::call(dara::backend::Interpreter& interpreter,
                          const std::vector<Value>& arguments) {
    auto instance = std::allocate_shared<Instance>(interpreter.get_allocator(),
                                                   shared_from_this());

    auto init_fields_res = this->init_default_fields(interpreter, instance);
    if (!init_fields_res) {
        return std::unexpected(init_fields_res.error());
    }

    Value method_val = find_method(this->name);

    // if (this->find_method(this->name)) {
    if (!std::holds_alternative<std::monostate>(method_val.data)) {
        auto init_res = instance->get(interpreter, this->name);
        if (!init_res) {
            return std::unexpected(init_res.error());
        }
        if (auto* callable_ptr =
                std::get_if<std::shared_ptr<dara::backend::Callable>>(
                    &init_res.value().data)) {
            auto call_res = (*callable_ptr)->call(interpreter, arguments);
            if (!call_res) {
                return std::unexpected(call_res.error());
            }
        }
    } else if (!arguments.empty()) {
        return std::unexpected(
            dara::error::InterpreterError("Expected 0 arguments but got " +
                                          std::to_string(arguments.size())));
    }
    return Value{instance};
}
*/
std::string Class::to_string() const { return "<class " + this->name + ">"; }
std::shared_ptr<Instance> Class::instantiate() {
	// Instance が完全型になっているため、make_shared が安全に展開される
	// (Instance が Class の参照を必要とする場合は shared_from_this()
	// を渡せます)
	return std::make_shared<Instance>(shared_from_this());
}

Result<Value> Class::get(Interpreter& interpreter, const std::string& name) {
	if (this->static_methods.contains(name)) {
		return Value{this->static_methods.at(name)};
	}
	return std::unexpected(
	    dara::error::InterpreterError("Undefined static property '" + name +
	                                  " on class '" + this->name + "'"));
}
/*
Result<Value> Class::get(const std::string& name) {
    if (name == "new") {
        auto constructor =
            std::make_shared<ClassConstructor>(shared_from_this());
        return Value{
            std::static_pointer_cast<dara::backend::Callable>(constructor)};
    }
    return std::unexpected(dara::error::InterpreterError("Undefined static
property '" + name + " on class '" + this->name +
                                            "'"));
}
*/

// std::optional<Value> Class::find_method(const std::string& name) {
Value Class::find_method(const std::string& name) const {
	if (this->methods.contains(name)) {
		return this->methods.at(name);
	}

	if (this->super) {
		Value super_method = this->super->find_method(name);
		if (!std::holds_alternative<std::monostate>(super_method.data)) {
			return super_method;
		}
		// return this->super->find_method(name);
	}

	for (const auto& mixin : this->mixins) {
		Value mixin_method = mixin->find_method(name);

		if (!std::holds_alternative<std::monostate>(mixin_method.data)) {
			return mixin_method;
		}
		// if (auto res = mixin->find_method(name)) {
		//	return res;
		// }
	}

	// return std::nullopt;
	return Value{.data = std::monostate{}};
}

}  // namespace dara::backend
