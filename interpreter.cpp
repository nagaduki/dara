/* interpreter.cpp */

// #pragma once
#include "interpreter.hpp"
// #include "environment.hpp"

#include <cmath>
#include <numeric>
#include <ranges>
#include <typeinfo>
#include <variant>

#include "callable.hpp"
#include "class.hpp"
#include "controlflow.hpp"
#include "error.hpp"
#include "instance.hpp"
// #include "native_function.hpp"
#include "builtin.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
// using EvalValue = std::variant<int, std::string>;
// using EvalValue = std::variant<char, int, std::string>;

namespace lox::backend {
/* in interpreter.cpp */
/*
std::string to_string(const lox::Value& val) {
    // inline std::string to_string(const Value& val) {
    return std::visit(
        overloaded{[](int v) { return std::to_string(v); },
                   [](const std::string& v) { return v; },
                   [](char v) { return std::string(1, v); },
                   [](bool v) {
                       if (v) {
                           return std::string("true");
                       } else {
                           return std::string("false");
                       }
                   },
                   [](std::monostate) { return std::string("nil"); },
                   [](const std::vector<lox::Value>& arr) -> std::string {
                       std::string s = "[";
                       for (size_t i = 0; i < arr.size(); ++i) {
                           if (i != arr.size() - 1) {
                               s += ", ";
                           }
                       }
                       s += "]";
                       return s;
                   },
                   [](std::shared_ptr<lox::backend::Callable> c) {
                       return c->to_string();
                   },  //(*)
                   //[](std::shared_ptr<lox::backend::NativeClock> c) { return
                   // c->to_string(); }, //(**)
                   [](const auto&) { return std::string("unknown"); }},
        val.data);
}
*/
/* is_truthy */
bool Interpreter::is_truthy(const lox::Value& val) {
	if (std::holds_alternative<std::monostate>(val.data)) {
		return false;
	}
	if (std::holds_alternative<bool>(val.data)) {
		return std::get<bool>(val.data);
	}
	return true;
}

/* Interpreter::visit_program */
Result<void> Interpreter::visit_program(const lox::Program* program) {
	// std::vector<std::unique_ptr<lox::Decl>> declarations =
	// program->declarations;
	for (const auto& decl : program->declarations) {
		auto val = this->exec(decl.get());
		if (!val) {
			// std::cerr << "Interpreter Error: " << val.error().message <<
			// "\n";
			return std::unexpected(val.error());
		}
	}
	return {};
}

/* visit_decl in interpreter.cpp */
Result<void> Interpreter::visit_decl(const lox::Decl* decl) {
	return std::visit(
	    overloaded{
	        [this, decl](const lox::VarDecl& d) -> Result<void> {
		        lox::Value initial_val = {};
		        if (d.initializer) {
			        auto res = this->visit_expr(d.initializer.get());
			        if (!res) {
				        return std::unexpected(res.error());
			        }
			        initial_val = res.value();
		        }

		        this->env.get()->define(d.name, initial_val);
		        // this.define(d.name, initial_val);
		        return {};
	        },
	        [this](const lox::ClassDecl& decl) -> Result<void> {
		        //
		        this->env->define(decl.name, Value{std::monostate{}});

		        // super class
		        std::shared_ptr<lox::runtime::Class> super = nullptr;
		        if (decl.super) {
			        auto super_val = this->visit_expr(decl.super.get());
			        if (!super_val) {
				        return std::unexpected(super_val.error());
			        }
			        if (auto* class_ptr =
			                std::get_if<std::shared_ptr<lox::runtime::Class>>(
			                    &super_val->data)) {
				        super = *class_ptr;
			        } else {
				        // throw InterpreterError("super class must bea class");
				        return std::unexpected(
				            InterpreterError("superclass must bea class"));
			        }
		        }

		        // mixi in
		        std::vector<std::shared_ptr<lox::runtime::Class>> mixins;
		        for (const auto& mixin_expr : decl.mixins) {
			        auto mixin_val = this->visit_expr(mixin_expr.get());
			        if (auto* class_ptr =
			                std::get_if<std::shared_ptr<lox::runtime::Class>>(
			                    &mixin_val->data)) {
				        mixins.push_back(*class_ptr);
			        } else {
				        return std::unexpected(
				            InterpreterError("Mixin must be a class"));
			        }
		        }

		        // method
		        std::unordered_map<std::string, Value> methods;
		        for (const auto& method_decl : decl.methods) {
			        if (auto* fn_expr = std::get_if<lox::FunctionExpr>(
			                &method_decl.function->value)) {
				        auto function =
				            std::make_shared<lox::backend::Function>(  //(*)
				                fn_expr, this->env);

				        methods[method_decl.name] = Value{function};
			        } else {
				        // throw InterpreterError( "method body must be
				        // FunctionExpr");
				        return std::unexpected(InterpreterError(
				            "Method body must be a FunctionExpr"));
			        }
		        }

		        auto cls = std::make_shared<lox::runtime::Class>(  //(**))
		            decl.name, std::move(methods));

		        cls->super = std::move(super);
		        cls->mixins = std::move(mixins);

		        this->env->assign(decl.name, Value{cls});

		        return {};
	        },
	        [this](const lox::MethodDecl& d) -> Result<void> { return {}; },
	        [this](const lox::TopLevelStmt& d) -> Result<void> {
		        return this->visit_stmt(d.stmt.get());
	        },
	        [](const auto& unknown_decl) -> std::string {
		        // RTTI (typeid)
		        // を使って、未知の型の本当の名前をコンソールに出力する
		        std::cout << "[DEBUG] Unknown Decl type: "
		                  << typeid(unknown_decl).name() << std::endl;

		        return "unimplemented_decl_node";
	        }},
	    decl->value);
};

/* in visit_stmt.cpp  */
Result<void> Interpreter::visit_stmt(const lox::Stmt* stmt) {
	return std::visit(
	    overloaded{
	        /*
	                    [this](const lox::ReturnStmt& s) -> Result<void> {
	                        lox::Value ret_val;
	                        ret_val.data = std::monostate{};

	                        if (s.value != nullptr) {
	                            auto res = this->visit_expr(s.value.get());
	                            if (!res) {
	                                return std::unexpected(res.error());
	                            }
	                            ret_val = res.value();
	                        }
	                        return {};
	        */
	        // Interpreter::visit_stmt の中の ReturnStmt 処理のイメージ

	        [&](const lox::ReturnStmt& ret_stmt) -> Result<void> {
		        lox::Value eval_val =
		            lox::Value{std::monostate{}};  // デフォルトは nil

		        // 1. 戻り値の式があれば、それを評価する (a + b など)
		        if (ret_stmt.value != nullptr) {
			        auto res = this->visit_expr(ret_stmt.value.get());
			        if (!res) return std::unexpected(res.error());
			        eval_val = std::move(res.value());
		        }

		        //  2. 超重要：ただ return するのではなく、例外として投げる！
		        // もしこれを書き忘れると、関数はジャンプせずにそのまま最後まで進み、
		        // Function::call の一番下にある return nil;
		        // に到達してしまいます。
		        throw lox::backend::ReturnException{std::move(eval_val)};
	        },
	        [&](const lox::SetStmt& stmt) -> Result<void> {
		        auto obj_res = this->visit_expr(stmt.object.get());
		        if (!obj_res) {
			        return std::unexpected(obj_res.error());
		        }
		        lox::Value obj = obj_res.value();

		        if (auto* instance =
		                std::get_if<std::shared_ptr<lox::runtime::Instance>>(
		                    &obj.data)) {
			        auto val_res = this->visit_expr(stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }

			        lox::Value val = val_res.value();
			        (*instance)->set(stmt.name, val);
			        return {};
		        }
		        return std::unexpected(
		            InterpreterError("only instances have fields"));
	        },
	        [&](const lox::BreakStmt& stmt) -> Result<void> {
		        throw lox::backend::BreakException{};
	        },
	        [&](const lox::ContinueStmt& stmt) -> Result<void> {
		        throw lox::backend::ContinueException{};
	        },

	        [this](const lox::ExprStmt& s) -> Result<void> {
		        auto res = this->visit_expr(s.expr.get());
		        if (!res) return std::unexpected(res.error());
		        return {/* next is here 612 */};
	        },
	        [this](const lox::PrintStmt& s) -> Result<void> {
		        auto res = this->visit_expr(s.expr.get());
		        if (!res) return std::unexpected(res.error());
		        this->out << lox::backend::to_string(res.value())
		                  << "\n";  //(**)
		        return {};
	        },
	        [this](const lox::IncStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name);
		        if (!val_res)
			        return std::unexpected(InterpreterError{
			            "undefined variable '" + stmt.name + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(InterpreterError{
			            "Operand '" + stmt.name + "' must be a number."});

		        auto assign_res =
		            this->env->assign(stmt.name, lox::Value{*num + 1});
		        if (!assign_res)
			        return std::unexpected(InterpreterError{
			            "undefined variable '" + stmt.name + "'"});
		        return {};
	        },
	        [this](const lox::DecStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name);
		        if (!val_res)
			        return std::unexpected(InterpreterError{
			            "undefined variable '" + stmt.name + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(InterpreterError{
			            "Operand '" + stmt.name + "' must be a number."});

		        auto assign_res =
		            this->env.get()->assign(stmt.name, lox::Value{*num - 1});
		        if (!assign_res)
			        return std::unexpected(InterpreterError{
			            "undefined variable '" + stmt.name + "'"});
		        return {};
	        },

	        [this](const lox::AssignStmt& s) -> Result<void> {
		        auto new_value_res = this->visit_expr(s.value.get());  //(*)
		        if (!new_value_res) {
			        PRINT_LINE();
			        return std::unexpected(new_value_res.error());
		        }
		        lox::Value new_value = new_value_res.value();

		        // auto success = this->env.assign(s.name, new_value);
		        auto success = this->env.get()->assign(s.name, new_value);
		        if (!success) {
			        PRINT_LINE();
			        return std::unexpected(InterpreterError{
			            "undefined variable '" + s.name + "'"});
		        }

		        return {};
	        },
	        [this](const lox::CompoundAssignStmt& cas) -> Result<void> {
		        auto current_value_opt = this->env->get(cas.name);  //(*)
		        if (!current_value_opt.has_value()) {
			        PRINT_LINE();
			        return std::unexpected(InterpreterError(
			            std::format("Underfined variable '{}'", cas.name)));
		        }
		        lox::Value current_value = current_value_opt.value();  // 1

		        auto rhs_res = this->visit_expr(cas.value.get());
		        if (!rhs_res) {
			        return std::unexpected(rhs_res.error());
		        }
		        lox::Value rhs_val = rhs_res.value();  // 2

		        auto eval_res = this->eval_infix(cas.op, current_value, rhs_val,
		                                         cas.value.get());
		        if (!eval_res) {
                    return std::unexpected(eval_res.error());
		        }
		        Value new_val = eval_res.value();
		        this->env->assign(cas.name, new_val);

		        return {};
	        },
	        [this](const lox::CompoundSetStmt& cas) -> Result<void> {
		        return {};
	        },
	        [this](const lox::IfStmt& s) -> Result<void> {
		        auto cond_res = this->visit_expr(s.condition.get());

		        // if (is_truthy(cond_res.value())) {
		        if (is_truthy(cond_res.value())) {
			        return this->visit_stmt(s.then_branch.get());
		        } else if (s.else_branch) {
			        return this->visit_stmt(s.else_branch.get());
		        }
		        return {};
	        },
	        [this](const lox::WhileStmt& stmt) -> Result<void> {
		        while (true) {
			        auto cond_res = this->visit_expr(stmt.condition.get());
			        if (!cond_res) return std::unexpected(cond_res.error());

			        if (!Interpreter::is_truthy(cond_res.value())) {
				        break;
			        }

			        // auto body_res = this->visit_stmt(stmt.body.get());
			        // if (!body_res) return std::unexpected(body_res.error());
			        try {
				        auto body_res = this->visit_stmt(stmt.body.get());
				        if (!body_res) return std::unexpected(body_res.error());
			        } catch (const lox::backend::BreakException&) {
				        break;
			        } catch (const lox::backend::ContinueException&) {
				        continue;
			        }
		        }

		        return {};
	        },
	        [this](const lox::ForInStmt& s) -> Result<void> {
		        auto iterable_res = this->visit_expr(s.iterable.get());
		        if (!iterable_res) {
			        return std::unexpected(iterable_res.error());
		        }

		        lox::Value iterable_val = iterable_res.value();

		        if (const auto* range_ptr =
		                std::get_if<lox::Range>(&iterable_val.data)) {
			        // 対象が Range の場合 (1..10 など)
			        for (int i = range_ptr->start; i <= range_ptr->end; ++i) {
				        // auto previous_env = this->env;
				        try {
					        // ループ本体を実行（深いネストで break
					        // されてもここに飛んでくる）
					        auto loop_res = this->execute_for_iteration(
					            s.loop_variable, lox::Value{i}, s.body.get());
					        if (!loop_res) {
						        // this->env = previous_env;
						        return loop_res;
					        }

				        } catch (const lox::backend::ContinueException&) {
					        // 何もせず環境を戻して、次のイテレーション（C++のループ）へ進む
					        // this->env = previous_env;
					        continue;
				        } catch (const lox::backend::BreakException&) {
					        // 環境を戻して、C++のループ自体を終了する
					        // this->env = previous_env;
					        break;
				        }

				        // auto loop_res = this->execute_for_iteration(
				        //    s.loop_variable, lox::Value{i}, s.body.get());
			        }
		        } else if (const auto* arr_ptr =
		                       std::get_if<std::vector<Value>>(
		                           &iterable_val.data)) {
			        // 対象が 配列 の場合 ([a, b, c] など)

			        for (const auto& elem : *arr_ptr) {
				        // auto previous_env = this->env;
				        try {
					        auto loop_res = this->execute_for_iteration(
					            s.loop_variable, elem, s.body.get());
					        if (!loop_res) {
						        return loop_res;
					        }

				        } catch (const lox::backend::ContinueException&) {
					        // 何もせず環境を戻して、次のイテレーション（C++のループ）へ進む
					        // this->env = previous_env;
					        continue;
				        } catch (const lox::backend::BreakException&) {
					        // 環境を戻して、C++のループ自体を終了する
					        // this->env = previous_env;
					        break;
				        }
			        }

		        } else {
			        return std::unexpected(
			            InterpreterError{"Object is not iterable."});
		        }

		        return {};
	        },
	        [this](const lox::BlockStmt& s) -> Result<void> {
		        auto previous_env = this->env;

		        this->env = std::make_shared<Environment>(previous_env);

		        struct EnvGuard {
			        std::shared_ptr<Environment>& current_env_ref;
			        std::shared_ptr<Environment> previous_env;
			        ~EnvGuard() { current_env_ref = previous_env; }
		        } guard{this->env, previous_env};

		        for (size_t i = 0; i < s.declarations.size(); ++i) {
			        auto decl_res = this->visit_decl(s.declarations[i].get());
			        if (!decl_res) {
				        this->env = previous_env;
				        return std::unexpected(decl_res.error());
			        }
		        }
		        this->env = previous_env;
		        return {};
	        },
	        [](auto&) -> Result<void> {
		        PRINT_LINE();
		        return std::unexpected(
		            InterpreterError("unimplemented statement execution"));
	        }},
	    stmt->value);
};

Result<void> Interpreter::execute_for_iteration(const std::string& var_name,
                                                const lox::Value& val,
                                                lox::Stmt* body) {
	auto loop_env = std::make_shared<Environment>(this->env);
	loop_env->define(var_name, val);

	/*
	auto previous_env = this->env;
	this->env = loop_env;

	auto result = this->visit_stmt(body);

	this->env = previous_env;
	*/
	EnvironmentGuard guard(this, loop_env);
	/*
	struct EnvGuard {
	    std::shared_ptr<Environment>& current_env_ref;
	    std::shared_ptr<Environment> previous_env;
	    ~EnvGuard() { current_env_ref = previous_env; }
	} guard{this->env, loop_env};
	*/

	auto result = this->visit_stmt(body);
	return result;
}

int Interpreter::factorial(int n) {
	if (n <= 1) return 1;
	auto rng = std::views::iota(1, n + 1);
	return std::accumulate(rng.begin(), rng.end(), 1LL, std::multiplies<int>());
}

/* eval_infix */
Result<lox::Value> Interpreter::eval_infix(InfixOperator op,
                                           const lox::Value& lhs_val,
                                           const lox::Value& rhs_val,
                                           const lox::Expr* expr) {
	return std::visit(
	    overloaded{
	        // for integer
	        [op, expr](int l, int r) -> Result<lox::Value> {
		        switch (op) {
			        case InfixOperator::Add:
				        return lox::Value{l + r};
			        case InfixOperator::Sub:
				        return lox::Value{l - r};
			        case InfixOperator::Mul:
				        return lox::Value{l * r};
			        case InfixOperator::Pow:
				        return lox::Value{static_cast<int>(std::pow(l, r))};
			        case InfixOperator::Div:
				        if (r == 0) {
					        return std::unexpected(
					            InterpreterError{"division by zero"});
					        //"division by zero", expr->line, expr->col});
				        }
				        return lox::Value{l / r};
			        case InfixOperator::Greater:
				        return lox::Value{l > r};
			        case InfixOperator::GreaterEqual:
				        return lox::Value{l >= r};
			        case InfixOperator::Less:
				        return lox::Value{l < r};
			        case InfixOperator::LessEqual:
				        return lox::Value{l <= r};
			        case InfixOperator::EqualEqual:
				        return lox::Value{l == r};
			        case InfixOperator::NotEqual:
				        return lox::Value{l != r};
			        case InfixOperator::Range:
				        if (typeid(l) == typeid(int) &&
				            typeid(r) == typeid(int)) {
					        return lox::Value{lox::Range{l, r}};
				        } else {
					        return std::unexpected(InterpreterError{
					            "Range operands must be integers."});
				        }
				        // return lox::Value{l != r};
			        default:
				        return std::unexpected(InterpreterError{std::format(
				            "[infix] unsupported operator error. {}",
				            expr->to_string())});
				        // expr->line, expr->col});
		        }
	        },
	        // for string
	        [op, expr](const std::string& l,
	                   const std::string& r) -> Result<lox::Value> {
		        /*
		        if (op == InfixOperator::Add) {
		            return lox::Value{l + r};
		        }
		        return std::unexpected(InterpreterError{
		            "unsupported operator for strings", expr->line, expr->col});
		        */
		        switch (op) {
			        case InfixOperator::Add:
				        return lox::Value{l + r};
			        case InfixOperator::EqualEqual:
				        return lox::Value{l == r};
			        case InfixOperator::NotEqual:
				        return lox::Value{l != r};
			        default:
				        return std::unexpected(InterpreterError{
				            "unsupported operator for strings"});
		        }
	        },
	        [op, expr](const std::string& l,
	                   const int r) -> Result<lox::Value> {
		        /*
		        if (op == InfixOperator::Add) {
		            return lox::Value{l + r};
		        }
		        return std::unexpected(InterpreterError{
		            "unsupported operator for strings", expr->line, expr->col});
		        */
		        switch (op) {
			        case InfixOperator::Add:
				        return lox::Value{l + std::to_string(r)};
			        default:
				        return std::unexpected(InterpreterError{
				            "unsupported operator for strings"});
		        }
	        },

	        // (int, int) or (string, string) 以外 i.e. mismatch
	        [expr](const auto&, const auto&) -> Result<lox::Value> {
		        return std::unexpected(
		            InterpreterError{"type mismatch in binary operation",
		                             expr->line, expr->col});
	        }},
	    lhs_val.data, rhs_val.data);
}

/* eval_prefix */
Result<lox::Value> Interpreter::eval_prefix(PrefixOperator op,
                                            const lox::Value& rhs_val,
                                            const lox::Expr* expr) {
	if (op == PrefixOperator::Not) {
		return lox::Value{!is_truthy(rhs_val)};
	}

	return std::visit(
	    overloaded{[op, expr](int r) -> Result<lox::Value> {
		               switch (op) {
			               case PrefixOperator::Neg:
				               return lox::Value{-r};
			               case PrefixOperator::Pos:
				               return lox::Value{+r};
				               /*
				              case PrefixOperator::Inc:
				                  return lox::Value{r + 1};
				              case PrefixOperator::Dec:
				                  return lox::Value{r - 1};
				               */
			               default:
				               return std::unexpected(InterpreterError{
				                   "[ pre] unsupported operator error."});
				               //"[ pre] unsupported operator error.",
				               // expr->line, expr->col});
		               }
	               },
	               [expr](const auto&) -> Result<lox::Value> {
		               return std::unexpected(InterpreterError{
		                   "type mismatch in prefix operation"});
		               // InterpreterError{"type mismatch in prefix operation",
		               // expr->line, expr->col});
	               }},
	    rhs_val.data);
}

/* eval_postfix */
Result<lox::Value> Interpreter::eval_postfix(PostfixOperator op,
                                             const lox::Value& lhs_val,
                                             const lox::Expr* expr) {
	return std::visit(
	    overloaded{[op, expr, this](int l) -> Result<lox::Value> {
		               switch (op) {
				               /*
				              case PostfixOperator::Inc:
				                  return lox::Value{l};
				              case PostfixOperator::Dec:
				                  return lox::Value{l};
				               */
			               case PostfixOperator::Fac:
				               if (l < 0) {
					               return std::unexpected(InterpreterError{
					                   "factorial of negative number",
					                   expr->line, expr->col});
				               }
				               return lox::Value{this->factorial(l)};
			               default:
				               return std::unexpected(InterpreterError{
				                   "[post] unsupported operator error.",
				                   expr->line, expr->col});
		               }
	               },
	               [expr](const auto&) -> Result<lox::Value> {
		               return std::unexpected(InterpreterError{
		                   "type mismatch in postfix operation", expr->line,
		                   expr->col});
	               }},
	    lhs_val.data);
}

/*
Result<lox::Value> Interpreter::eval_mixfix(MixfixOperator op,
                                            const lox::Value& lhs_val,
                                            const lox::Expr* expr) {
    //
    // Result<lox::CallExpr> callee_value = this->env->get(lhs_val.data);;
}
*/

Result<lox::Value> Interpreter::visit_expr(const lox::Expr* expr) {
	return std::visit(
	    overloaded{
	        [](const lox::BoolExpr& e) -> Result<lox::Value> {
		        return lox::Value{e.value};
	        },
	        [](const lox::IntExpr& e) -> Result<lox::Value> {
		        return lox::Value{e.value};
	        },
	        [this](const lox::ThisExpr& e) -> Result<lox::Value> {
		        return this->env->get("this").value();
	        },

	        [](const lox::StringExpr& e) -> Result<lox::Value> {
		        return lox::Value{e.value};
	        },
	        [](const lox::CharExpr& e) -> Result<lox::Value> {
		        return lox::Value{e.value};
	        },
	        [this](const lox::FunctionExpr& e) -> Result<lox::Value> {
		        std::shared_ptr<Environment> current_env = this->env;
		        auto function = std::make_shared<lox::backend::Function>(
		            &e, current_env);  //(*)

		        return lox::Value{function};
	        },
	        [this](const lox::ArrayExpr& e) -> Result<lox::Value> {
		        std::vector<lox::Value> evaluated_elements;
		        for (const auto& elem : e.elements) {
			        auto evaluated_val = this->visit_expr(elem.get());  //(*)
			        if (!evaluated_val) {
				        return std::unexpected(evaluated_val.error());
			        }
			        evaluated_elements.push_back(evaluated_val.value());
		        }
		        return lox::Value{evaluated_elements};
	        },
	        [this](const lox::IndexExpr& e) -> Result<lox::Value> {
		        auto array_res = this->visit_expr(e.array.get());
		        if (!array_res) {
			        return std::unexpected(array_res.error());
		        }

		        auto index_res = this->visit_expr(e.index.get());
		        if (!index_res) {
			        return std::unexpected(index_res.error());
		        }

		        const auto* arr_ptr =
		            std::get_if<std::vector<Value>>(&array_res.value().data);
		        if (!arr_ptr) {
			        return std::unexpected(
			            InterpreterError{"Only arrays can be indexed"});
		        }

		        const auto* index_ptr =
		            std::get_if<int>(&index_res.value().data);
		        if (!index_ptr) {
			        return std::unexpected(
			            InterpreterError{"Array index must be integer"});
		        }
		        int index = *index_ptr;
		        return (*arr_ptr)[index];
	        },

	        //[this, expr](const IdentifierExpr& e) -> Result<Value> {
	        [this, expr](const lox::VarExpr& e) -> Result<lox::Value> {
		        // auto val_opt = this->env.get(e.name);
		        auto val_opt = this->env.get()->get(e.name);
		        if (!val_opt) {
			        PRINT_LINE();
			        return std::unexpected(
			            // InterpreterError{"undefined variable '" + e.name
			            // +
			            // "'",
			            InterpreterError{"undefined variable '" + e.name + "'",
			                             expr->line, expr->col});
		        }
		        // return val_opt.value();
		        return lox::Value{val_opt.value()};
	        },
	        [this, expr](const lox::CallExpr& e) -> Result<lox::Value> {
		        // auto val_opt = this->env.get(e.name);
		        // auto val_opt = this->env->get(*e.callee);
		        auto val_opt = this->visit_expr(e.callee.get());
		        if (!val_opt) {
			        return std::unexpected(val_opt.error());
		        }
		        lox::Value callee = val_opt.value();

		        std::vector<lox::Value> arguments;
		        for (const auto& arg_expr : e.arguments) {
			        auto arg_res = this->visit_expr(arg_expr.get());
			        if (!arg_res) return std::unexpected(arg_res.error());
			        arguments.push_back(arg_res.value());  //(A)
		        }

		        return std::visit(
		            overloaded{
		                [&](std::shared_ptr<lox::backend::Callable> callable)
		                    -> Result<lox::Value> {
			                // auto* callable_ptr =
			                //     std::get_if<std::shared_ptr<Callable>>(
			                //         &val_opt.value().data);  //(B)

			                // if (!callable_ptr) {
			                // if (!callable_ptr) {
			                //     return std::unexpected(InterpreterError{
			                //         "can only call functions and classes'"});
			                // }

			                // std::shared_ptr<Callable> function = *callable;
			                //  std::shared_ptr<Callable> function =
			                //  *callable_ptr;

			                // if (arguments.size() != function->arity()) {
			                if (arguments.size() != callable->arity()) {
				                return std::unexpected(InterpreterError{
				                    "Expected " +
				                    std::to_string(callable->arity()) +
				                    " argument but got " +
				                    std::to_string(arguments.size()) + "."});
			                }
			                return callable->call(*this, arguments);
		                },

		                [&](std::shared_ptr<lox::runtime::Class> cls)
		                    -> Result<lox::Value> {
			                std::shared_ptr<lox::runtime::Instance> instance =
			                    cls->instantiate();

			                if (cls->methods.contains("new")) {
				                auto init_res = instance->get("new");
				                if (!init_res) {
					                return std::unexpected(init_res.error());
				                }
				                if (auto* callable_ptr =
				                        std::get_if<std::shared_ptr<
				                            lox::backend::Callable>>(
				                            &init_res.value().data)) {
					                auto constructor = *callable_ptr;

					                if (arguments.size() !=
					                    constructor->arity()) {
						                return std::unexpected(InterpreterError{
						                    "Expected " +
						                    std::to_string(
						                        constructor->arity()) +
						                    " argument but got " +
						                    std::to_string(arguments.size()) +
						                    "."});
					                }

					                auto call_res =
					                    constructor->call(*this, arguments);
					                if (!call_res) {
						                return std::unexpected(
						                    call_res.error());
					                }
				                }
			                } else {
				                if (!arguments.empty()) {
					                return std::unexpected(InterpreterError(
					                    "Expected 0 arguments bu got " +
					                    std::to_string(arguments.size())));
				                }
			                }

			                return lox::Value{instance};
		                },
		                [&](auto&&) -> Result<lox::Value> {
			                return std::unexpected(InterpreterError(
			                    "Can only call function and classes"));
		                }},
		            callee.data);
	        },
	        [this, expr](const lox::GetExpr& e) -> Result<lox::Value> {
		        auto obj_res = this->visit_expr(e.object.get());
		        if (!obj_res) {
			        return std::unexpected(obj_res.error());
		        }
		        lox::Value obj = obj_res.value();

		        return std::visit(
		            overloaded{
		                // 1. インスタンスに対するプロパティ/メソッドアクセス
		                [&](std::shared_ptr<lox::runtime::Instance> instance)
		                    -> Result<lox::Value> {
			                return instance->get(e.name);
		                },
		                //  2. クラスに対するスタティックメソッドアクセス
		                // (Person.new など)
		                [&](std::shared_ptr<lox::runtime::Class> cls)
		                    -> Result<lox::Value> { return cls->get(e.name); },
		                // 3. それ以外の型にはドットアクセス不可
		                [&](auto&&) -> Result<lox::Value> {
			                return std::unexpected(InterpreterError(
			                    "only instances and classes have properties."));
		                }},
		            obj.data);

		        /*
		        if (auto* instance =
		                std::get_if<std::shared_ptr<lox::runtime::Instance>>(
		                    &obj.data)) {
		            return (*instance)->get(e.name);  //(*)
		        }
		        return std::unexpected(
		            InterpreterError("only instances ave properties"));
		        */
	        },

	        [this, expr](const lox::InfixOpExpr& e) -> Result<lox::Value> {
		        auto lhs_res = this->visit_expr(e.lhs.get());
		        if (!lhs_res) return std::unexpected(lhs_res.error());

		        auto rhs_res = this->visit_expr(e.rhs.get());
		        if (!rhs_res) return std::unexpected(rhs_res.error());

		        return this->eval_infix(e.op, lhs_res.value(), rhs_res.value(),
		                                expr);
	        },
	        [this, expr](const lox::LogicalOpExpr& e) -> Result<lox::Value> {
		        auto lhs_res = this->visit_expr(e.lhs.get());
		        if (!lhs_res) return lhs_res;

		        if (e.op == LogicalOperator::Or) {
			        if (Interpreter::is_truthy(lhs_res.value())) {
				        return lhs_res;
			        }
		        } else {  // LogicalOperator::And
			        if (!Interpreter::is_truthy(lhs_res.value())) {
				        return lhs_res;
			        }
		        }

		        // return std::visit(*this, expr.)
		        return this->visit_expr(e.rhs.get());
	        },

	        //},
	        [this, expr](const lox::PrefixOpExpr& e) -> Result<lox::Value> {
		        auto rhs_res = this->visit_expr(e.rhs.get());
		        if (!rhs_res) return std::unexpected(rhs_res.error());

		        return this->eval_prefix(e.op, rhs_res.value(), expr);
	        },
	        [this, expr](const lox::PostfixOpExpr& e) -> Result<lox::Value> {
		        auto lhs_res = this->visit_expr(e.lhs.get());
		        if (!lhs_res) return std::unexpected(lhs_res.error());

		        return this->eval_postfix(e.op, lhs_res.value(), expr);
	        },

	        [expr](const auto&) -> Result<lox::Value> {
		        PRINT_LINE();
		        return std::unexpected(
		            InterpreterError{"unimplemented expression evaluation",
		                             expr->line, expr->col});
		        // InterpreterError {"unimplemented expression
		        // evaluation"});::
	        },
	        /*
	        [expr](const auto&) -> Result<lox::Value> {
	            PRINT_LINE();
	            return std::unexpected(
	                InterpreterError{"unimplemented expression evaluation",
	                                 expr->line, expr->col});
	            // InterpreterError {"unimplemented expression
	            // evaluation"});::
	        },
	        */
	    },
	    expr->value);
}

}  // namespace lox::backend

/*
int Evaluator::visit_expr(const lox::Expr* expr) {  // 実装は .cpp へ
    return std::visit(
        overloaded{[](const lox::IntExpr& expr) { return expr.value; },

                   [this](const lox::InfixOpExpr& expr) {
                       int left = visit_expr(expr.lhs.get());
                       int right =
                           visit_expr(expr.rhs.get());  // rhs.get() に修正
                       switch (expr.op) {
                           case InfixOperator::Add:
                               return left + right;
                           case InfixOperator::Sub:
                               return left - right;
                           case InfixOperator::Mul:
                               return left * right;
                           case InfixOperator::Div:
                               return left / right;
                           case InfixOperator::Pow:
                               return static_cast<int>(std::pow(left, right));
                           case InfixOperator::Assign:

                               // return 0;
                               return right;
                       }
                       return 0;
                   },
                   [this](const lox::PrefixOpExpr& expr) {
                       std::string op_str;
                       int right = visit_expr(expr.rhs.get());
                       switch (expr.op) {
                           case PrefixOperator::Pos:
                               return right;
                           case PrefixOperator::Neg:
                               return -1 * right;
                           case PrefixOperator::Not:
                               return ~right;
                               break;  //(***)
                       }
                       return 0;
                   },

                   [this](const lox::PostfixOpExpr& expr) {
                       std::string op_str;
                       int left = visit_expr(expr.lhs.get());
                       switch (expr.op) {
                           case PostfixOperator::Fac:
                               return factorial(left);
                       }
                       return 0;  // returnが抜けていたのを追加
                   },             // ← ここにカンマが必要でした
                   [](const auto&) { return 0; }},
        expr->value);
}
int Evaluator::factorial(int n) {
    if (n <= 1) return 1;
    auto rng = std::views::iota(1, n + 1);
    return std::accumulate(rng.begin(), rng.end(), 1LL, std::multiplies<int>());
}
*/
