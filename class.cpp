/* class.cpp */
#include "class.hpp"

#include "callable.hpp"
#include "instance.hpp"  //  ここで Instance の完全な定義を読み込む

namespace dara::runtime {

size_t Class::arity() const {
    dara::Value method_val = find_method(this->name); //(**)
	//if (auto method_val = const_cast<Class*>(this)->find_method(this->name)) {
	if (!std::holds_alternative<std::monostate>(method_val.data)) {
		if (auto* callable_ptr =
		        std::get_if<std::shared_ptr<dara::backend::Callable>>(
		            &method_val.data)) {
			return (*callable_ptr)->arity();
		}
	}
    return 0;
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

Result<dara::Value> call(dara::backend::Interpreter& interpreter,
                        const std::vector<dara::Value>& arguments) override {
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
            InterpreterError("Expected 0 arguments but got " +
                             std::to_string(arguments.size())));
    }
    return Value{instance};
}

std::string to_string() const override {
    return "<constructor for " + cls->name + ">";
}
};
*/

Result<dara::Value> Class::call(dara::backend::Interpreter& interpreter,
                               const std::vector<dara::Value>& arguments) {
	auto instance = std::make_shared<Instance>(shared_from_this());
    dara::Value method_val = find_method(this->name); //(**)

	//if (this->find_method(this->name)) {
	if (!std::holds_alternative<std::monostate>(method_val.data)) {
		auto init_res = instance->get(this->name);
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
		    InterpreterError("Expected 0 arguments but got " +
		                     std::to_string(arguments.size())));
	}
	return Value{instance};
}

std::string Class::to_string() const { return "<class " + this->name + ">"; }
std::shared_ptr<Instance> Class::instantiate() {
	// Instance が完全型になっているため、make_shared が安全に展開される
	// (Instance が Class の参照を必要とする場合は shared_from_this()
	// を渡せます)
	return std::make_shared<Instance>(shared_from_this());
}

Result<dara::Value> Class::get(const std::string& name) {
	if (this->static_methods.contains(name)) {
		return Value{this->static_methods.at(name)};
	}
	return std::unexpected(InterpreterError("Undefined static property '" +
	                                        name + " on class '" + this->name +
	                                        "'"));
}
/*
Result<dara::Value> Class::get(const std::string& name) {
    if (name == "new") {
        auto constructor =
            std::make_shared<ClassConstructor>(shared_from_this());
        return Value{
            std::static_pointer_cast<dara::backend::Callable>(constructor)};
    }
    return std::unexpected(InterpreterError("Undefined static property '" +
                                            name + " on class '" + this->name +
                                            "'"));
}
*/

// std::optional<Value> Class::find_method(const std::string& name) {
dara::Value Class::find_method(const std::string& name) const {
	if (this->methods.contains(name)) {
		return this->methods.at(name);
	}

	if (this->super) {
		dara::Value super_method = this->super->find_method(name);
		if (!std::holds_alternative<std::monostate>(super_method.data)) {
			return super_method;
		}
		// return this->super->find_method(name);
	}

	for (const auto& mixin : this->mixins) {
		dara::Value mixin_method = mixin->find_method(name);

		if (!std::holds_alternative<std::monostate>(mixin_method.data)) {
			return mixin_method;
		}
		// if (auto res = mixin->find_method(name)) {
		//	return res;
		// }
	}

	//return std::nullopt;
	return dara::Value{.data = std::monostate{}};
}

}  // namespace dara::runtime
