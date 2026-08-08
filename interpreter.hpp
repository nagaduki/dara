/* interpreter.hpp */
#pragma once
#include <utility>

#include "ast.hpp"
#include "value.hpp"
// #include "declaration.hpp"
#include "environment.hpp"
#include "builtin.hpp"

// //
template <typename T>
using Result = std::expected<T, InterpreterError>;

namespace lox::backend {

std::string to_string(const lox::Value& val);

class Interpreter {
   public:
	~Interpreter() {
		if (this->env) {
			//std::cout << "[DEBUG] Interpreter is dying..." << std::endl;
			this->env->clear();
		}
	}
	Result<lox::Value> eval(const lox::Expr* expr) {
		return visit_expr(expr);
	}  //(*)
	Result<void> exec(const lox::Program* program) {
		return visit_program(program);
	}  //(**)
	Result<void> exec(const lox::Decl* decl) {
		return visit_decl(decl);
	}  //(***)
	Result<void> visit_program(const lox::Program* program);
	Result<void> visit_decl(const lox::Decl* decl);
	Result<void> visit_stmt(const lox::Stmt* expr);
	Result<lox::Value> visit_expr(const lox::Expr* expr);

	explicit Interpreter(std::ostream& out_stream = std::cout)
	    : out(out_stream) { /* (*) */
		this->env->define(
		    "clock",
		    lox::Value{.data = std::make_shared<lox::backend::NativeClock>()});

		this->env->define(
		    "len",
		    lox::Value{.data = std::make_shared<lox::backend::NativeLen>()});
    }


	std::optional<lox::Value> get_variable_for_test(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::optional<lox::Value> get_variable(const std::string& name) {
		// return this->env.get()->get(name);
		return this->env->get(name);
	}
	std::shared_ptr<Environment> get_environment() { return this->env; }

	void set_environment(std::shared_ptr<Environment> env) { this->env = env; }

   private:
	std::ostream& out;

	std::shared_ptr<Environment> env = std::make_shared<Environment>();

	Result<void> execute_for_iteration(const std::string& var_name,
	                                   const lox::Value& val, lox::Stmt* body);

	Result<lox::Value> eval_infix(InfixOperator, const lox::Value&,
	                              const lox::Value&, /* 1 */
	                              const lox::Expr*);
	Result<lox::Value> eval_prefix(PrefixOperator, const lox::Value&,
	                               const lox::Expr*); /* 2 */
	Result<lox::Value> eval_postfix(PostfixOperator, const lox::Value&,
	                                const lox::Expr*); /* 3 */
	Result<lox::Value> eval_mixfix(MixfixOperator, const lox::Value&,
	                               const lox::Expr*); /* 4 */
	int factorial(int n);
	bool is_truthy(const lox::Value& val);
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

}  // namespace lox::backend
