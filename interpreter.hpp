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

// template <typename T>
// using Result = std::expected<T, dara::error::InterpreterError>;

namespace dara::backend {

std::string to_string(const Value& val);

class Interpreter {
   public:
	dara::backend::Allocator get_allocator() { return alloc; }
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
        this->module_cache.clear();
        this->module_envs.clear();
        this->globals.reset();
	}
	std::unordered_map<std::string, std::shared_ptr<dara::backend::Instance>>
	    module_cache;
	std::vector<dara::ast::Program> module_asts;
	std::vector<std::filesystem::path> dir_stack;
    dara::core::SourceManager source_manager;
	dara::ast::Result<Value> load_module(const std::string& path);
	dara::ast::Result<Value> eval(const dara::ast::Expr* expr) {
		return visit_expr(expr);
	}
	dara::ast::Result<void> exec(const dara::ast::Program* program) {
		return visit_program(program);
	}
	dara::ast::Result<void> exec(const dara::ast::Decl* decl) {
		return visit_decl(decl);
	}
	dara::ast::Result<void> visit_program(const dara::ast::Program* program);
	dara::ast::Result<void> visit_decl(const dara::ast::Decl* decl);
	dara::ast::Result<void> visit_stmt(const dara::ast::Stmt* expr);
	dara::ast::Result<Value> visit_expr(const dara::ast::Expr* expr);

	explicit Interpreter(std::ostream& out_stream = std::cout)
	    : out(out_stream),
	      arena(),
	      alloc(&arena),
	      globals(std::allocate_shared<Environment>(alloc, alloc, nullptr)),
	      env(globals) { /* (*) */
		this->define_native_classes();
		this->define_native_functions();

		/*
		this->env->define(
		    "clock",
		    Value{.data =
		std::make_shared<dara::backend::BuiltinClock>()});

		this->env->define(
		    "len",
		    Value{.data = std::make_shared<dara::backend::BuiltinLen>()});

		this->env->define(
		    "type",
		    Value{.data =
		std::make_shared<dara::backend::BuiltinType>()});

		this->env->define(
		    "assert",
		    Value{.data =
		                   std::make_shared<dara::backend::BuiltinAssert>()});
		this->env->define(
		    "props",
		    Value{.data =
		std::make_shared<dara::backend::BuiltinProps>()}); this->env->define(
		    "id",
		    Value{.data = std::make_shared<dara::backend::BuiltinId>()});
		this->env->define(
		    "is_a",
		    Value{.data =
		std::make_shared<dara::backend::BuiltinIs_A>()}); this->env->define(
		    "is_proper",
		    Value{.data =
		                   std::make_shared<dara::backend::BuiltinIs_Proper>()});
		*/
	}

	std::optional<Value> get_variable_for_test(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::optional<Value> get_variable(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::shared_ptr<Environment> get_environment() { return this->env; }

	void set_environment(std::shared_ptr<Environment> env) { this->env = env; }
	
    uint16_t get_id() { return this->id; } //(***)

	bool is_truthy(const Value& val);

   private:
	std::ostream& out;
	std::pmr::monotonic_buffer_resource arena;
	dara::backend::Allocator alloc;

	// std::shared_ptr<Environment> env = std::make_shared<Environment>();
	// std::shared_ptr<Environment> globals = std::make_shared<Environment>();
	std::shared_ptr<Environment> globals;
	// std::shared_ptr<Environment> env = globals;
	std::shared_ptr<Environment> env;
	std::vector<std::shared_ptr<Environment>> module_envs;
    uint16_t id; //(****)

	dara::ast::Result<void> execute_for_iteration(const std::string& var_name,
	                                              const Value& val,
	                                              dara::ast::Stmt* body);

	dara::ast::Result<Value> eval_infix(dara::lexer::InfixOperator,
	                                    const Value&, const Value&, /* 1 */
	                                    const dara::ast::Expr*);
	dara::ast::Result<Value> eval_prefix(dara::lexer::PrefixOperator,
	                                     const Value&,
	                                     const dara::ast::Expr*); /* 2 */
	dara::ast::Result<Value> eval_postfix(dara::lexer::PostfixOperator,
	                                      const Value&,
	                                      const dara::ast::Expr*); /* 3 */
	dara::ast::Result<Value> eval_mixfix(dara::lexer::MixfixOperator,
	                                     const Value&,
	                                     const dara::ast::Expr*); /* 4 */
	int factorial(int n);

	void define_native_classes() {
		std::unordered_map<std::string, Value> object_methods;
		// auto object_class = std::make_shared<dara::runtime::Class>(
		//     "Object", std::move(object_methods));
		// this->env->define("Object", Value{object_class});

		// std::unordered_map<std::string, Value> object_methods;

		// object_methods["type"] = Value{.data =
		// std::make_shared<BuiltinType>()};
		object_methods["type"] =
		    Value{.data = std::allocate_shared<BuiltinType>(this->alloc)};
		object_methods["props"] =
		    // Value{.data = std::make_shared<BuiltinProps>()};
		    Value{.data = std::allocate_shared<BuiltinProps>(this->alloc)};
		// object_methods["id"] = Value{.data = std::make_shared<BuiltinId>()};
		object_methods["id"] =
		    Value{.data = std::allocate_shared<BuiltinId>(this->alloc)};
		// auto object_class = std::make_shared<dara::backend::Class>(  //***
		auto object_class = std::allocate_shared<dara::backend::Class>(  //***
		    this->alloc, "Object", nullptr,
		    std::vector<std::shared_ptr<dara::backend::Class>>{},
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
		    //Value{.data = std::make_shared<dara::backend::BuiltinClock>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinClock>(this->alloc)});

		// this->env->define(
		this->globals->define(
		    "len",
		    //Value{.data = std::make_shared<dara::backend::BuiltinLen>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinLen>(this->alloc)});

		// this->env->define(
		this->globals->define(
		    "type",
		    //Value{.data = std::make_shared<dara::backend::BuiltinType>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinType>(this->alloc)});

		// this->env->define(
		this->globals->define(
		    "assert",
		    //Value{.data = std::make_shared<dara::backend::BuiltinAssert>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinAssert>(this->alloc)});
		// this->env->define(
		this->globals->define(
		    "props",
		    //Value{.data = std::make_shared<dara::backend::BuiltinProps>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinProps>(this->alloc)});
		// this->env->define(
		this->globals->define(
		    //"id", Value{.data = std::make_shared<dara::backend::BuiltinId>()});
		    "id", Value{.data = std::allocate_shared<dara::backend::BuiltinId>(this->alloc)});
		// this->env->define(
		this->globals->define(
		    "is_a",
		    //Value{.data = std::make_shared<dara::backend::BuiltinIs_A>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinIs_A>(this->alloc)});
		// this->env->define(
		this->globals->define(
		    "is_proper",
		    //Value{.data = std::make_shared<dara::backend::BuiltinIs_Proper>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinIs_Proper>(this->alloc)});
		this->globals->define(
		    "require",
		    //Value{.data = std::make_shared<dara::backend::BuiltinRequire>()});
		    Value{.data = std::allocate_shared<dara::backend::BuiltinRequire>(this->alloc)});
	}

	// bool is_truthy(const Value& val);
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
