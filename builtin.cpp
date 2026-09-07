#include "builtin.hpp"

#include <chrono>
#include <memory>

#include "callable.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "value.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

namespace dara::backend {

std::expected<dara::Value, InterpreterError> BuiltinClock::call(
    Interpreter& interpreter, const std::vector<dara::Value>& arguments) {
	auto now = std::chrono::system_clock::now().time_since_epoch();
	int seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count();

	return dara::Value{.data = seconds};
}

std::expected<dara::Value, InterpreterError> BuiltinLen::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);
	if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
		return dara::Value{.data = static_cast<int>(arr->size())};
	}
	if (const auto* str = std::get_if<std::string>(&arg.data)) {
		return dara::Value{.data = static_cast<int>(str->length())};
	}
	return std::unexpected(InterpreterError("Arguments must be an vector"));
}

std::expected<dara::Value, InterpreterError> BuiltinType::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);
	if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
		// return dara::Value{.data = static_cast<int>(arr->size())};
		return dara::Value{.data = "vector"};
	}
	if (const auto* str = std::get_if<std::string>(&arg.data)) {
		// return dara::Value{.data = static_cast<int>(str->length())};
		// return dara::Value{.data = static_cast<int>(str->length())};
		return dara::Value{.data = "string"};
	}
	if (const auto* str = std::get_if<int>(&arg.data)) {
		return dara::Value{.data = "integer"};
	}
	if (const auto* str = std::get_if<char>(&arg.data)) {
		return dara::Value{.data = "char"};
	}
	if (const auto* str = std::get_if<bool>(&arg.data)) {
		return dara::Value{.data = "bool"};
	}
	if (const auto* str =
	        std::get_if<std::shared_ptr<dara::backend::Callable>>(&arg.data)) {
		return dara::Value{.data = "function"};
	}
	if (const auto* str =
	        std::get_if<std::shared_ptr<dara::runtime::Class>>(&arg.data)) {
		return dara::Value{.data = "class"};
	}

	if (const auto* inst =
	        std::get_if<std::shared_ptr<dara::runtime::Instance>>(&arg.data)) {
		// return dara::Value{.data = "instance"};
		return dara::Value{.data = (*inst)->get_class()->name};
	}
	return dara::Value{.data = std::string("unknown")};
}

std::expected<dara::Value, InterpreterError> BuiltinAssert::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!interpreter.is_truthy(arguments[0])) {  //
		std::string msg = "Assertion failed";
		if (auto* str = std::get_if<std::string>(&arguments[1].data)) {
			// PRINT_LINE();
			msg += ": " + *str;
		}
		PRINT_LINE();
		return std::unexpected(InterpreterError(msg));
	}
	PRINT_LINE();
	std::cout << "Assert passed" << std::endl;
	return dara::Value{.data = true};
}

/*
std::expected<dara::Value, InterpreterError> BuiltinProps::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
    const auto& arg = arguments.at(0);

    std::vector<dara::Value> prop_names;

    if (const auto* instance =
            std::get_if<std::shared_ptr<dara::runtime::Instance>>(&arg.data)) {
        // instance->get();
        std::vector<std::string> names = (*instance)->get_property_names();

        for (const std::string& name : names) {
            prop_names.push_back(dara::Value{.data = name});
        }
        return dara::Value{.data = prop_names};
    }

    return std::unexpected(InterpreterError("Argument must be an instance"));
}
*/

std::expected<dara::Value, InterpreterError> BuiltinProps::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	// std::vector<dara::Value> prop_names;
	std::set<std::string> prop_names;

	if (const auto* instance =
	        std::get_if<std::shared_ptr<dara::runtime::Instance>>(&arg.data)) {
		// instance->get();
		std::vector<std::string> names = (*instance)->get_property_names();

		for (const auto& name : (*instance)->get_property_names()) {
			prop_names.insert(name);
		}

		auto current_class = (*instance)->get_class();
		while (current_class != nullptr) {
			for (const auto& [name, method] : current_class->methods) {
				prop_names.insert(name);
			}

			for (const auto& mixin : current_class->mixins) {
				for (const auto& [name, method] : mixin->methods) {
					prop_names.insert(name);
				}
			}

			current_class = current_class->super;
		}
		std::vector<Value> result_array;
		for (const auto& name : prop_names) {
			result_array.push_back(dara::Value{.data = name});
		}

		return dara::Value{.data = result_array};
		// return dara::Value{.data = prop_names};
	}

	return std::unexpected(InterpreterError("Arguments must be an instance"));
}

std::expected<dara::Value, InterpreterError> BuiltinId::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	if (auto* instance =
	        std::get_if<std::shared_ptr<dara::runtime::Instance>>(&arg.data)) {
		return dara::Value{.data = std::format("{:p}", (void*)instance->get())};
	}
	return dara::Value{.data = std::string("not a reference type")};
}

bool BuiltinIs_A::check_inheritance(
    std::shared_ptr<dara::runtime::Class> current,
    std::shared_ptr<dara::runtime::Class> target) {
	if (!current) {
		return false;
	};

	if (current == target) {
		return true;
	}

	for (const auto& mixin : current->mixins) {
		if (check_inheritance(mixin, target)) {
			return true;
		}
	}

	return check_inheritance(current->super, target);
}

std::expected<dara::Value, InterpreterError> BuiltinIs_A::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (auto* inst = std::get_if<std::shared_ptr<dara::runtime::Instance>>(
	        &arguments[0].data)) {
		if (auto* target_cls =
		        std::get_if<std::shared_ptr<dara::runtime::Class>>(
		            &arguments[1].data)) {
			bool result = check_inheritance((*inst)->get_class(), *target_cls);
			return dara::Value{.data = result};
		}
		return std::unexpected(InterpreterError(
		    "Second argument to 'is_a' must be a class or mixin"));
	}
	return dara::Value{.data = false};
}

std::expected<dara::Value, InterpreterError> BuiltinIs_Proper::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (auto* instance = std::get_if<std::shared_ptr<dara::runtime::Instance>>(
	        &arguments[0].data)) {
		if (auto* target_class =
		        std::get_if<std::shared_ptr<dara::runtime::Class>>(
		            &arguments[1].data)) {
			if ((*instance)->get_class() == *target_class) {
				// return true;
				return dara::Value{.data = true};
			} else {
				return dara::Value{.data = false};
			}
		}
		return std::unexpected(InterpreterError(
		    "Second argument to 'is_proper' must be a class or mixin"));
	}
	return dara::Value{.data = false};
}

std::expected<dara::Value, InterpreterError> BuiltinRequire::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!std::holds_alternative<std::string>(arguments[0].data)) {
		return std::unexpected(
		    InterpreterError("require() expects a string path"));
	}
	// std::string path = std::get<std::string>(arguments[0].data);
	std::string req_path_str = std::get<std::string>(arguments[0].data);
	std::filesystem::path target_path(req_path_str);

	if (!target_path.is_absolute() && !interpreter.dir_stack.empty()) {
		target_path = interpreter.dir_stack.back() / target_path;
	}

	std::error_code ec;
	target_path = std::filesystem::weakly_canonical(target_path, ec);

	if (std::filesystem::is_directory(target_path)) {
		return std::unexpected(InterpreterError("Cannot require a directory: " +
		                                        target_path.string()));
	}

	//std::cout << "[DEBUG] require resolved to: " << target_path.string()
	//          << std::endl;

	if (ec) {
		return std::unexpected(
		    InterpreterError("Invalid path: " + req_path_str));
	}

	return interpreter.load_module(target_path.string());
}

}  // namespace dara::backend
