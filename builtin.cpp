#include "builtin.hpp"

#include <chrono>
#include <memory>

#include "callable.hpp"
#include "error.hpp"
#include "interpreter.hpp"
#include "value.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace dara::backend {

using namespace dara::ast;

std::expected<Value, dara::error::InterpreterError> BuiltinClock::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	auto now = std::chrono::system_clock::now().time_since_epoch();
	int seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count();

	return Value{.data = seconds};
}

std::expected<Value, dara::error::InterpreterError> BuiltinLen::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);
	// if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
	/*
	if (const auto* arr = std::get_if<Array>(&arg.data)) {
	    return Value{.data = static_cast<int>(arr->size())};
	}
	if (const auto* str = std::get_if<String>(&arg.data)) {
	    return Value{.data = static_cast<int>(str->length())};
	}
	return std::unexpected(
	    dara::error::InterpreterError("Arguments must be an vector"));
	*/
	return std::visit(
	    overloaded{[](const Array& arr) -> Result<Value> {
		               return Value{static_cast<int>(arr.size())};
	               },
	               [](const String& str) -> Result<Value> {
		               return Value{static_cast<int>(str.length())};
	               },
	               [](auto&&) -> Result<Value> {
		               return std::unexpected(dara::error::InterpreterError(
		                   "Argument must be an Array or String"));
	               }},
	    arg.data);
}

std::expected<Value, dara::error::InterpreterError> BuiltinType::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);
	return std::visit(
	    overloaded{
	        [&](int) -> Result<Value> {
		        return Value{String("Integer", interpreter.get_allocator())};
	        },
	        [&](double) -> Result<Value> {
		        return Value{String("Double", interpreter.get_allocator())};
	        },
	        [&](char) -> Result<Value> {
		        return Value{String("Char", interpreter.get_allocator())};
	        },
	        [&](bool) -> Result<Value> {
		        return Value{String("Bool", interpreter.get_allocator())};
	        },
	        [&](std::shared_ptr<dara::backend::Callable>) -> Result<Value> {
		        return Value{String("Function", interpreter.get_allocator())};
	        },
	        [&](std::shared_ptr<dara::backend::Class>) -> Result<Value> {
		        return Value{String("Class", interpreter.get_allocator())};
	        },
	        [&](std::shared_ptr<dara::backend::Instance> instance)
	            -> Result<Value> {
		        return Value{String(instance->get_class()->name.c_str(),
		                            interpreter.get_allocator())};
	        },
	        [&](const String&) -> Result<Value> {
		        return Value{String("String", interpreter.get_allocator())};
	        },
	        [&](const Array&) -> Result<Value> {
		        return Value{String("Array", interpreter.get_allocator())};
	        },
	        [&](auto&&) -> Result<Value> {
		        return Value{String("Unknown", interpreter.get_allocator())};
	        }},
	    arg.data);
}

/*
std::expected<Value, dara::error::InterpreterError> BuiltinType::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
    const auto& arg = arguments.at(0);
    if (const auto* arr = std::get_if<std::vector<Value>>(&arg.data)) {
        // return Value{.data = static_cast<int>(arr->size())};
        return Value{.data = "vector"};
    }
    if (const auto* str = std::get_if<std::string>(&arg.data)) {
        // return Value{.data = static_cast<int>(str->length())};
        // return Value{.data = static_cast<int>(str->length())};
        return Value{.data = "string"};
    }
    //
    //if (const auto* str = std::get_if<int>(&arg.data)) {
    //	return Value{.data = "integer"};
    //}
    //
    if (const auto* str = std::get_if<int>(&arg.data)) {
        return Value{.data = "integer"};
    }
    if (const auto* str = std::get_if<char>(&arg.data)) {
        return Value{.data = "char"};
    }
    if (const auto* str = std::get_if<bool>(&arg.data)) {
        return Value{.data = "bool"};
    }
    if (const auto* str =
            std::get_if<std::shared_ptr<dara::backend::Callable>>(&arg.data))
{ return Value{.data = "function"};
    }
    if (const auto* str =
            std::get_if<std::shared_ptr<dara::backend::Class>>(&arg.data))
{ return Value{.data = "class"};
    }

    if (const auto* inst =
            std::get_if<std::shared_ptr<dara::backend::Instance>>(&arg.data))
{
        // return Value{.data = "instance"};
        return Value{.data = (*inst)->get_class()->name};
    }
    return Value{.data = std::string("unknown")};
}
*/

std::expected<Value, dara::error::InterpreterError> BuiltinAssert::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!interpreter.is_truthy(arguments[0])) {  //
		std::string msg = "Assertion failed";
		// if (auto* str = std::get_if<std::string>(&arguments[1].data)) {
		if (auto* str = std::get_if<String>(&arguments[1].data)) {
			// PRINT_LINE();
			msg += ": " + *str;
		}
		PRINT_LINE();
		return std::unexpected(dara::error::InterpreterError(msg));
	}
	PRINT_LINE();
	std::cout << "Assert passed" << std::endl;
	return Value{.data = true};
}

/*
std::expected<Value, dara::error::InterpreterError> BuiltinProps::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
    const auto& arg = arguments.at(0);

    std::vector<Value> prop_names;

    if (const auto* instance =
            std::get_if<std::shared_ptr<dara::backend::Instance>>(&arg.data))
{
        // instance->get();
        std::vector<std::string> names =
(*instance)->get_property_names();

        for (const std::string& name : names) {
            prop_names.push_back(Value{.data = name});
        }
        return Value{.data = prop_names};
    }

    return std::unexpected(dara::error::InterpreterError("Argument must
be an instance"));
}
*/

std::expected<Value, dara::error::InterpreterError> BuiltinProps::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	// std::vector<Value> prop_names;
	std::set<std::string> prop_names;

	if (const auto* instance =
	        std::get_if<std::shared_ptr<dara::backend::Instance>>(&arg.data)) {
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
		// std::vector<Value> result_array;
		Array result_array;
		for (const auto& name : prop_names) {
			String pmr_name(name.c_str(), interpreter.get_allocator());
			result_array.push_back(Value{.data = pmr_name});
		}

		return Value{.data = result_array};
		// return Value{.data = prop_names};
	}

	return std::unexpected(
	    dara::error::InterpreterError("Arguments must be an instance"));
}

std::expected<Value, dara::error::InterpreterError> BuiltinId::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	const auto& arg = arguments.at(0);

	if (auto* instance =
	        std::get_if<std::shared_ptr<dara::backend::Instance>>(&arg.data)) {
		// return Value{.data = std::format("{:p}", (void*)instance->get())};
		return Value{String(std::format("{:p}", (void*)instance->get()),
		                    interpreter.get_allocator())};
	}
	// return Value{.data = std::string("not a reference type")};
	return Value{String("not a reference type", interpreter.get_allocator())};
}

bool BuiltinIs_A::check_inheritance(
    std::shared_ptr<dara::backend::Class> current,
    std::shared_ptr<dara::backend::Class> target) {
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

std::expected<Value, dara::error::InterpreterError> BuiltinIs_A::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (auto* inst = std::get_if<std::shared_ptr<dara::backend::Instance>>(
	        &arguments[0].data)) {
		if (auto* target_cls =
		        std::get_if<std::shared_ptr<dara::backend::Class>>(
		            &arguments[1].data)) {
			bool result = check_inheritance((*inst)->get_class(), *target_cls);
			return Value{.data = result};
		}
		return std::unexpected(dara::error::InterpreterError(
		    "Second argument to 'is_a' must be a class or mixin"));
	}
	return Value{.data = false};
}

std::expected<Value, dara::error::InterpreterError> BuiltinIs_Proper::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (auto* instance = std::get_if<std::shared_ptr<dara::backend::Instance>>(
	        &arguments[0].data)) {
		if (auto* target_class =
		        std::get_if<std::shared_ptr<dara::backend::Class>>(
		            &arguments[1].data)) {
			if ((*instance)->get_class() == *target_class) {
				// return true;
				return Value{.data = true};
			} else {
				return Value{.data = false};
			}
		}
		return std::unexpected(dara::error::InterpreterError(
		    "Second argument to 'is_proper' must be a class or mixin"));
	}
	return Value{.data = false};
}

std::expected<Value, dara::error::InterpreterError> BuiltinRequire::call(
    Interpreter& interpreter, const std::vector<Value>& arguments) {
	if (!std::holds_alternative<String>(arguments[0].data)) { //(*)
		return std::unexpected(
		    dara::error::InterpreterError("require() expects a string path"));
	}
    const String& pmr_path = std::get<String>(arguments[0].data);
    std::string req_path_str(pmr_path.c_str());

	// std::string path = std::get<std::string>(arguments[0].data);
	//std::string req_path_str = std::get<std::string>(arguments[0].data);
    //std::string req_path_str = std::get<std::string>(arguments[0].data);//(**)
	std::filesystem::path target_path(req_path_str);

	if (!target_path.is_absolute() && !interpreter.dir_stack.empty()) {
		target_path = interpreter.dir_stack.back() / target_path;
	}

	std::error_code ec;
	target_path = std::filesystem::weakly_canonical(target_path, ec);

	if (std::filesystem::is_directory(target_path)) {
		return std::unexpected(dara::error::InterpreterError(
		    "Cannot require a directory: " + target_path.string()));
	}

	// std::cout << "[DEBUG] require resolved to: " <<
	// target_path.string()
	//           << std::endl;

	if (ec) {
		return std::unexpected(
		    dara::error::InterpreterError("Invalid path: " + req_path_str)); //(*)
	}

	return interpreter.load_module(target_path.string());
}

}  // namespace dara::backend
