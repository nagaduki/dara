#include "class.hpp"

#include "callable.hpp"
#include "instance.hpp"  //  ここで Instance の完全な定義を読み込む

namespace lox::runtime {

class ClassConstructor : public lox::backend::Callable {
   private:
	std::shared_ptr<Class> cls;

   public:
	explicit ClassConstructor(std::shared_ptr<Class> cls)
	    : cls(std::move(cls)) {}

	size_t arity() const override {
		if (cls->methods.contains("initialize")) {
			auto method_val = cls->methods.at("initialize");
			if (auto* callable_ptr =
			        std::get_if<std::shared_ptr<lox::backend::Callable>>(
			            &method_val.data)) {
				return (*callable_ptr)->arity();
			}
		}
		return 0;
	}

	Result<lox::Value> call(lox::backend::Interpreter& interpreter,
	                        const std::vector<lox::Value>& arguments) override {
		auto instance = std::make_shared<Instance>(cls);

		if (cls->methods.contains("initialize")) {
			auto init_res = instance->get("initialize");
			if (!init_res) {
				return std::unexpected(init_res.error());
			}
			if (auto* callable_ptr =
			        std::get_if<std::shared_ptr<lox::backend::Callable>>(
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

std::shared_ptr<Instance> Class::instantiate() {
	// Instance が完全型になっているため、make_shared が安全に展開される
	// (Instance が Class の参照を必要とする場合は shared_from_this()
	// を渡せます)
	return std::make_shared<Instance>(shared_from_this());
}

Result<lox::Value> Class::get(const std::string& name) {
	if (name == "new") {
		auto constructor =
		    std::make_shared<ClassConstructor>(shared_from_this());
		return Value{
		    std::static_pointer_cast<lox::backend::Callable>(constructor)};
	}
	return std::unexpected(InterpreterError("Undefined static property '" +
	                                        name + " on class '" + this->name +
	                                        "'"));
}

}  // namespace lox::runtime
