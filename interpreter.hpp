/* interpreter.hpp */
#pragma once
#include <filesystem>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "value.hpp"
// #include "declaration.hpp"
#include "builtin.hpp"
#include "environment.hpp"

// //
template <typename T>
using Result = std::expected<T, InterpreterError>;

namespace dara::backend {

std::string to_string(const dara::Value& val);

class Interpreter {
   public:
	Interpreter(Interpreter&&) = default;  //(***)
	~Interpreter() {
		if (this->env) {
			// std::cout << "[DEBUG] Interpreter is dying..." << std::endl;
			this->env->clear();
		}
		if (this->globals) {
			this->globals->clear();
		}
		for (auto& m_env : this->module_envs) {
			if (m_env) m_env->clear();
		}
	}
	std::unordered_map<std::string, std::shared_ptr<dara::runtime::Instance>>
	    module_cache;
	std::vector<dara::Program> module_asts;
	std::vector<std::filesystem::path> dir_stack;
	Result<dara::Value> load_module(const std::string& path);
	Result<dara::Value> eval(const dara::Expr* expr) { return visit_expr(expr); }
	Result<void> exec(const dara::Program* program) {
		return visit_program(program);
	}
	Result<void> exec(const dara::Decl* decl) { return visit_decl(decl); }
	Result<void> visit_program(const dara::Program* program);
	Result<void> visit_decl(const dara::Decl* decl);
	Result<void> visit_stmt(const dara::Stmt* expr);
	Result<dara::Value> visit_expr(const dara::Expr* expr);

	explicit Interpreter(std::ostream& out_stream = std::cout)
	    : out(out_stream) { /* (*) */

		this->define_native_classes();
		this->define_native_functions();

		/*
		this->env->define(
		    "clock",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinClock>()});

		this->env->define(
		    "len",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinLen>()});

		this->env->define(
		    "type",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinType>()});

		this->env->define(
		    "assert",
		    dara::Value{.data =
		                   std::make_shared<dara::backend::BuiltinAssert>()});
		this->env->define(
		    "props",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinProps>()});
		this->env->define(
		    "id",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinId>()});
		this->env->define(
		    "is_a",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinIs_A>()});
		this->env->define(
		    "is_proper",
		    dara::Value{.data =
		                   std::make_shared<dara::backend::BuiltinIs_Proper>()});
		*/
	}

	std::optional<dara::Value> get_variable_for_test(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::optional<dara::Value> get_variable(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::shared_ptr<Environment> get_environment() { return this->env; }

	void set_environment(std::shared_ptr<Environment> env) { this->env = env; }

	bool is_truthy(const dara::Value& val);

   private:
	std::ostream& out;

	// std::shared_ptr<Environment> env = std::make_shared<Environment>();
	std::shared_ptr<Environment> globals = std::make_shared<Environment>();
	std::shared_ptr<Environment> env = globals;
	std::vector<std::shared_ptr<Environment>> module_envs;

	Result<void> execute_for_iteration(const std::string& var_name,
	                                   const dara::Value& val, dara::Stmt* body);

	Result<dara::Value> eval_infix(InfixOperator, const dara::Value&,
	                              const dara::Value&, /* 1 */
	                              const dara::Expr*);
	Result<dara::Value> eval_prefix(PrefixOperator, const dara::Value&,
	                               const dara::Expr*); /* 2 */
	Result<dara::Value> eval_postfix(PostfixOperator, const dara::Value&,
	                                const dara::Expr*); /* 3 */
	Result<dara::Value> eval_mixfix(MixfixOperator, const dara::Value&,
	                               const dara::Expr*); /* 4 */
	int factorial(int n);

	void define_native_classes() {
		std::unordered_map<std::string, Value> object_methods;
		// auto object_class = std::make_shared<dara::runtime::Class>(
		//     "Object", std::move(object_methods));
		// this->env->define("Object", Value{object_class});

		// std::unordered_map<std::string, Value> object_methods;

		object_methods["type"] = Value{.data = std::make_shared<BuiltinType>()};
		object_methods["props"] =
		    Value{.data = std::make_shared<BuiltinProps>()};
		object_methods["id"] = Value{.data = std::make_shared<BuiltinId>()};
		auto object_class = std::make_shared<dara::runtime::Class>(  //***
		    "Object", nullptr,
		    std::vector<std::shared_ptr<dara::runtime::Class>>{},
		    std::move(object_methods));

		// auto object_class = std::make_shared<dara::runtime::Class>("Object",
		// std::move(object_methods));
		// this->env->define("Object", Value{object_class});
		this->globals->define("Object", Value{object_class});
	}

	void define_native_functions() {
		//
		// this->env->define(
		this->globals->define(
		    "clock",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinClock>()});

		// this->env->define(
		this->globals->define(
		    "len",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinLen>()});

		// this->env->define(
		this->globals->define(
		    "type",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinType>()});

		// this->env->define(
		this->globals->define(
		    "assert",
		    dara::Value{.data =
		                   std::make_shared<dara::backend::BuiltinAssert>()});
		// this->env->define(
		this->globals->define(
		    "props",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinProps>()});
		// this->env->define(
		this->globals->define(
		    "id",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinId>()});
		// this->env->define(
		this->globals->define(
		    "is_a",
		    dara::Value{.data = std::make_shared<dara::backend::BuiltinIs_A>()});
		// this->env->define(
		this->globals->define(
		    "is_proper",
		    dara::Value{.data =
		                   std::make_shared<dara::backend::BuiltinIs_Proper>()});
		this->globals->define(
		    "require",
		    dara::Value{.data =
		                   std::make_shared<dara::backend::BuiltinRequire>()});
	}

	// bool is_truthy(const dara::Value& val);
	struct EnvironmentGuard {
		Interpreter* interpreter;
		std::shared_ptr<Environment> previous_env;

		EnvironmentGuard(Interpreter* interp,
		                 std::shared_ptr<Environment> new_env)
		    : interpreter(interp), previous_env(interp->env) {
			interpreter->env = new_env;  // 切り替え
		}

		~EnvironmentGuard() {
			interpreter->env = previous_env;  // デストラクタで必ず復元！
		}
	};
};

}  // namespace dara::backend
