/* interpreter.cpp */

// #pragma once
#include "interpreter.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
// #include "environment.hpp"

#include <cmath>
#include <memory>
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
#include "environment.hpp"
#include "parser.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

namespace dara::backend {

using namespace dara::lexer;
using namespace dara::ast;

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
// using EvalValue = std::variant<int, std::string>;
// using EvalValue = std::variant<char, int, std::string>;

/* in interpreter.cpp */
/* is_truthy */
bool Interpreter::is_truthy(const Value& val) {
	if (std::holds_alternative<std::monostate>(val.data)) {
		return false;
	}
	if (std::holds_alternative<bool>(val.data)) {
		return std::get<bool>(val.data);
	}
	return true;
}

template <typename T>
concept Numeric = std::is_same_v<T, int> || std::is_same_v<T, double>;

Result<Value> Interpreter::load_module(const std::string& path) {
	if (this->module_cache.contains(path)) {
		return Value{.data = this->module_cache.at(path)};  // (A)
	}

	std::ifstream file(path);  
	if (!file.is_open()) {
		return std::unexpected(dara::error::InterpreterError(
		    "Could not open module file: " + path));
	}
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string source_code = buffer.str();

    /*
	Source s(source_code.c_str());
	dara::frontend::Parser p(&s);
    */

    uint16_t file_id = this->source_manager.load_file(path, source_code);
    dara::core::Source* s = this->source_manager.get_source(file_id);
    dara::frontend::Parser p(s);

	auto program_res = p.program();
	if (!program_res) {
		return std::unexpected(
		    dara::error::InterpreterError("syntax error in module '" + path +
		                                  "': " + program_res.error().message));
	}


	// dara::Program program = std::move(program_res.value());
	this->module_asts.push_back(std::move(program_res.value()));
	const dara::ast::Program& program = this->module_asts.back();

	struct DirGuard {
		Interpreter* interp;
		DirGuard(Interpreter* i, std::filesystem::path p) : interp(i) {
			interp->dir_stack.push_back(p);
		}
		~DirGuard() { interp->dir_stack.pop_back(); }
	};

	DirGuard dir_guard(this, std::filesystem::path(path).parent_path());

	// auto module_env = std::make_shared<Environment>(this->globals);
	auto module_env =
	    // std::allocate_shared<Environment>(this->alloc, this->globals);
	    std::allocate_shared<Environment>(this->alloc, this->alloc,
	                                      this->globals);
	this->module_envs.push_back(module_env);

	auto previous_env = this->env;
	this->env = module_env;

	for (const auto& decl : program.declarations) {
		auto val = this->exec(decl.get());
		if (!val) {
			this->env = previous_env;
			return std::unexpected(val.error());
		}
	}
	this->env = previous_env;

	// auto dummy_class = std::make_shared<dara::backend::Class>(
	auto dummy_class = std::allocate_shared<dara::backend::Class>(
	    this->alloc, "Module_" + path, nullptr,
	    std::vector<std::shared_ptr<dara::backend::Class>>{},
	    std::unordered_map<std::string, Value>{},
	    std::unordered_map<std::string, Value>{});
	auto module_instance =
	    // std::make_shared<dara::backend::Instance>(dummy_class);
	    // std::make_shared<dara::backend::Instance>(this->alloc, dummy_class);
	    std::allocate_shared<dara::backend::Instance>(this->alloc, dummy_class);

	for (const auto& [name, val] : module_env->get_all_values()) {
		module_instance->set(name, val);
	}

	this->module_cache[path] = module_instance;
	return Value{.data = module_instance};  //(B)
}

/* Interpreter::visit_program */
Result<void> Interpreter::visit_program(const Program* program) {
	// std::vector<std::unique_ptr<dara::Decl>> declarations =
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
Result<void> Interpreter::visit_decl(const Decl* decl) {
	return std::visit(
	    overloaded{
	        [this, decl](const VarDecl& d) -> Result<void> {
		        Value initial_val = {};
		        if (d.initializer) {
			        auto res = this->visit_expr(d.initializer.get());
			        if (!res) {
				        return std::unexpected(res.error());
			        }
			        initial_val = res.value();
		        }

		        this->env.get()->define(d.name.lexeme, initial_val);
		        // this.define(d.name, initial_val);
		        return {};
	        },

	        // ClassDecl
	        [this, decl](const ClassDecl& d) -> Result<void> {
		        //
		        this->env->define(d.name.lexeme, Value{std::monostate{}});  // (A)

		        // super class
		        std::shared_ptr<dara::backend::Class> super = nullptr;
		        if (d.super) {
			        auto super_val = this->visit_expr(d.super.get());
			        if (!super_val) {
				        return std::unexpected(super_val.error());
			        }
			        if (auto* class_ptr = std::get_if<
			                std::shared_ptr<dara::backend::Class>>(  // (C)
			                &super_val->data)) {
				        super = *class_ptr;
			        } else {
				        // throw dara::error::InterpreterError("super class must
				        // bea class");
				        return std::unexpected(dara::error::InterpreterError(
				            "superclass must be a class", decl->span));  //(***)
			        }
		        } else {
			        if (d.name.lexeme != "Object") {
				        auto obj_val_opt = this->env->get("Object");
				        if (obj_val_opt) {
					        if (auto* obj_class = std::get_if<
					                std::shared_ptr<dara::backend::Class>>(
					                &obj_val_opt.value().data)) {
						        super = *obj_class;
					        }
				        }
			        }
		        }
		        // mixi in
		        std::vector<std::shared_ptr<dara::backend::Class>> mixins;
		        for (const auto& mixin_expr : d.mixins) {
			        auto mixin_val = this->visit_expr(mixin_expr.get());
			        if (!mixin_val) {
				        return std::unexpected(mixin_val.error());
			        }
			        if (auto* class_ptr =
			                std::get_if<std::shared_ptr<dara::backend::Class>>(
			                    &mixin_val->data)) {
				        mixins.push_back(*class_ptr);
			        } else {
				        return std::unexpected(dara::error::InterpreterError(
				            "Mixin must be a class", decl->span));
			        }
		        }

		        // method
		        std::unordered_map<std::string, Value> methods;
		        for (const auto& method_decl : d.methods) {
			        if (auto* fn_expr = std::get_if<FunctionExpr>(
			                &method_decl.function->value)) {
				        auto function =
				            // std::make_shared<dara::backend::Function>(
				            std::allocate_shared<dara::backend::Function>(
				                // fn_expr, this->env);
				                this->alloc, fn_expr, this->env);

				        methods[method_decl.name.lexeme] = Value{function};
			        } else {
				        // throw dara::error::InterpreterError( "method body
				        // must be FunctionExpr");
				        return std::unexpected(dara::error::InterpreterError(
				            "Method body must be a FunctionExpr", decl->span));
			        }
		        }

		        // static method
		        std::unordered_map<std::string, Value> static_methods;
		        for (const auto& static_method_decl : d.static_methods) {
			        if (auto* static_fn_expr = std::get_if<FunctionExpr>(
			                &static_method_decl.function->value)) {
				        auto function =
				            // std::make_shared<dara::backend::Function>(
				            std::allocate_shared<dara::backend::Function>(
				                // static_fn_expr, this->env);
				                this->alloc, static_fn_expr, this->env);

				        static_methods[static_method_decl.name.lexeme] =
				            Value{.data = function};
			        } else {
				        //
				        return std::unexpected(dara::error::InterpreterError(
				            "Static Method body must be a FunctionExpr",
				            decl->span));
			        }
		        }
		        // auto cls = std::make_shared<dara::backend::Class>(
		        auto cls = std::allocate_shared<dara::backend::Class>(
		            this->alloc, d.name.lexeme, std::move(super), std::move(mixins),
		            std::move(methods), std::move(static_methods));

		        // cls->super = std::move(super);
		        // cls->mixins = std::move(mixins);

		        this->env->assign(d.name.lexeme, Value{cls});  //(B)
		        return {};
	        },

	        [this](const MethodDecl& d) -> Result<void> { return {}; },
	        [this](const TopLevelStmt& d) -> Result<void> {
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
Result<void> Interpreter::visit_stmt(const Stmt* stmt) {
	return std::visit(
	    overloaded{
	        /*
	                    [this](const dara::ReturnStmt& s) -> Result<void> {
	                        Value ret_val;
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

	        [&](const ReturnStmt& ret_stmt) -> Result<void> {
		        Value eval_val = Value{std::monostate{}};  // デフォルトは nil

		        // 1. 戻り値の式があれば、それを評価する (a + b など)
		        if (ret_stmt.value != nullptr) {
			        auto res = this->visit_expr(ret_stmt.value.get());
			        if (!res) return std::unexpected(res.error());
			        eval_val = std::move(res.value());
		        }

		        //  2. 超重要：ただ return
		        //  するのではなく、例外として投げる！
		        // もしこれを書き忘れると、関数はジャンプせずにそのまま最後まで進み、
		        // Function::call の一番下にある return nil;
		        // に到達してしまいます。
		        throw dara::backend::ReturnException{std::move(eval_val)};
	        },
	        // (*****) //
	        [&, stmt](const SetStmt& set_stmt) -> Result<void> {
		        auto obj_res = this->visit_expr(set_stmt.object.get());
		        if (!obj_res) {
			        auto obj_err = obj_res.error();
			        // obj_err.span = stmt->span;
			        //return std::unexpected(obj_err);
			        return std::unexpected(obj_res.error().enrich_span(stmt->span));
		        }
		        // return obj_res.error();

		        Value obj = obj_res.value();

		        if (auto* instance = std::get_if<
		                std::shared_ptr<dara::backend::Instance>>(  // D
		                &obj.data)) {
			        auto val_res = this->visit_expr(set_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }

			        Value val = val_res.value();
			        (*instance)->set(set_stmt.name.lexeme, val);
			        return {};
		        }
		        return std::unexpected(dara::error::InterpreterError(
		            "only instances have fields", stmt->span));
	        },
	        [&](const BreakStmt& stmt) -> Result<void> {
		        throw dara::backend::BreakException{};
	        },
	        [&](const ContinueStmt& stmt) -> Result<void> {
		        throw dara::backend::ContinueException{};
	        },

	        [this](const ExprStmt& s) -> Result<void> {
		        auto res = this->visit_expr(s.expr.get());
		        if (!res) return std::unexpected(res.error());
		        return {/* next is here 612 */};
	        },
	        [this](const PrintStmt& s) -> Result<void> {
		        auto res = this->visit_expr(s.expr.get());
		        if (!res) return std::unexpected(res.error());
		        this->out << dara::backend::to_string(res.value())
		                  << "\n";  //(**)
		        return {};
	        },
	        [this](const IncStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name.lexeme);
		        if (!val_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(dara::error::InterpreterError{
			            "Operand '" + stmt.name.lexeme + "' must be a number."});

		        auto assign_res = this->env->assign(stmt.name.lexeme, Value{*num + 1});
		        if (!assign_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        return {};
	        },
	        [this](const DecStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name.lexeme);
		        if (!val_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(dara::error::InterpreterError{
			            "Operand '" + stmt.name.lexeme + "' must be a number."});

		        auto assign_res =
		            this->env.get()->assign(stmt.name.lexeme, Value{*num - 1});
		        if (!assign_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        return {};
	        },

	        [this](const AssignStmt& s) -> Result<void> {
		        auto new_value_res = this->visit_expr(s.value.get());  //(*)
		        if (!new_value_res) {
			        //PRINT_LINE();
			        return std::unexpected(new_value_res.error());
		        }
		        Value new_value = new_value_res.value();

		        // auto success = this->env.assign(s.name, new_value);
		        auto success = this->env.get()->assign(s.name.lexeme, new_value);
		        if (!success) {
			        //PRINT_LINE();
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + s.name.lexeme + "'"});
		        }

		        return {};
	        },
	        /*
	        [&](const dara::CmeompoundSetStmt& stmt) -> Result<void> {
	            auto obj_res = this->visit_expr(stmt.object.get());
	            if (!obj_res) {
	                return std::unexpected(obj_res.error());
	            }

	            Value obj = obj_res.value();
	            if (auto* instance =
	                    std::get_if<std::shared_ptr<dara::backend::Instance>>(
	                        &obj.data)) {
	                // value評価
	                auto val_res = this->visit_expr(stmt.value.get());
	                if (!val_res) {
	                    return std::unexpected(val_res.error());
	                }

	                Value val = val_res.value();
	                auto current_val = (*instance)->get(stmt.name);
	                if (!current_val) {
	                    auto eval_res = this->eval_infix(stmt.op,
	        current_val.value(), val_res.value(), stmt.value.get()); Value
	        new_val = eval_res.value();
	                    (*instance)->set(stmt.name, new_val);
	                }
	                return {};
	            }
	            return std::unexpected(
	                dara::error::InterpreterError("only instances have
	        fields"));
	        },
	        */
	        [this](const CompoundAssignStmt& cas) -> Result<void> {
		        auto current_value_opt = this->env->get(cas.name.lexeme);  //(*)
		        if (!current_value_opt.has_value()) {
			        PRINT_LINE();
			        return std::unexpected(dara::error::InterpreterError(
			            std::format("Underfined variable '{}'", cas.name.lexeme)));
		        }
		        Value current_value = current_value_opt.value();  // 1

		        auto rhs_res = this->visit_expr(cas.value.get());
		        if (!rhs_res) {
			        return std::unexpected(rhs_res.error());
		        }
		        Value rhs_val = rhs_res.value();  // 2

		        auto eval_res = this->eval_infix(cas.op, current_value, rhs_val,
		                                         cas.value.get());
		        if (!eval_res) {
			        return std::unexpected(eval_res.error());
		        }
		        Value new_val = eval_res.value();
		        this->env->assign(cas.name.lexeme, new_val);

		        return {};
	        },
	        // css //
	        [this, stmt](const CompoundSetStmt& c_stmt) -> Result<void> {
		        // 1. object 評価
		        auto obj_res = this->visit_expr(c_stmt.object.get());
		        if (!obj_res) {
			        return std::unexpected(obj_res.error());
		        }
		        Value obj = obj_res.value();

		        if (auto* instance =
		                std::get_if<std::shared_ptr<dara::backend::Instance>>(
		                    &obj.data)) {
			        // current property value
			        auto current_val_opt = (*instance)->get(*this, c_stmt.name.lexeme);
			        if (!current_val_opt) {
				        //auto val_opt_err = current_val_opt.error();
				        //val_opt_err.span = stmt->span;
				        //return std::unexpected(val_opt_err);
				        //return std::unexpected(val_opt_err.enrich_span(stmt->span)); //(A)
				        return std::unexpected(current_val_opt.error().enrich_span(c_stmt.name.span)); //(A)



				        // return std::unexpected(dara::error::InterpreterError(
				        //     std::format("Undefined property '{}'",
				        //     c_stmt.name)));
			        }
			        Value current_val = current_val_opt.value();

			        auto val_res = this->visit_expr(c_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }
			        Value val = val_res.value();

			        // 3 rhs
			        auto eval_res = this->eval_infix(c_stmt.op, current_val,
			                                         val, c_stmt.value.get());
			        if (!eval_res) {
				        return std::unexpected(eval_res.error());
			        }

			        Value new_val = eval_res.value();
			        (*instance)->set(c_stmt.name.lexeme, new_val);
			        //auto set_res = (*instance)->set(c_stmt.name, new_val);
                    
			        return {};
		        }
		        return std::unexpected(dara::error::InterpreterError(
		            "only instance have fields", stmt->span));
	        },

	        [this](const IfStmt& s) -> Result<void> {
		        auto cond_res = this->visit_expr(s.condition.get());

		        // if (is_truthy(cond_res.value())) {
		        if (is_truthy(cond_res.value())) {
			        return this->visit_stmt(s.then_branch.get());
		        } else if (s.else_branch) {
			        return this->visit_stmt(s.else_branch.get());
		        }
		        return {};
	        },
	        [this](const WhileStmt& stmt) -> Result<void> {
		        while (true) {
			        auto cond_res = this->visit_expr(stmt.condition.get());
			        if (!cond_res) return std::unexpected(cond_res.error());

			        if (!Interpreter::is_truthy(cond_res.value())) {
				        break;
			        }

			        // auto body_res = this->visit_stmt(stmt.body.get());
			        // if (!body_res) return
			        // std::unexpected(body_res.error());
			        try {
				        auto body_res = this->visit_stmt(stmt.body.get());
				        if (!body_res) return std::unexpected(body_res.error());
			        } catch (const dara::backend::BreakException&) {
				        break;
			        } catch (const dara::backend::ContinueException&) {
				        continue;
			        }
		        }

		        return {};
	        },
	        [this](const ForInStmt& s) -> Result<void> {
		        auto iterable_res = this->visit_expr(s.iterable.get());
		        if (!iterable_res) {
			        return std::unexpected(iterable_res.error());
		        }

		        Value iterable_val = iterable_res.value();

		        if (const auto* range_ptr =
		                std::get_if<dara::backend::Range>(&iterable_val.data)) {
			        // 対象が Range の場合 (1..10 など)
			        for (int i = range_ptr->start; i <= range_ptr->end; ++i) {
				        // auto previous_env = this->env;
				        try {
					        // ループ本体を実行（深いネストで break
					        // されてもここに飛んでくる）
					        auto loop_res = this->execute_for_iteration(
					            s.loop_variable.lexeme, Value{i}, s.body.get());
					        if (!loop_res) {
						        // this->env = previous_env;
						        return loop_res;
					        }

				        } catch (const dara::backend::ContinueException&) {
					        // 何もせず環境を戻して、次のイテレーション（C++のループ）へ進む
					        // this->env = previous_env;
					        continue;
				        } catch (const dara::backend::BreakException&) {
					        // 環境を戻して、C++のループ自体を終了する
					        // this->env = previous_env;
					        break;
				        }

				        // auto loop_res = this->execute_for_iteration(
				        //    s.loop_variable, Value{i}, s.body.get());
			        }
		        } else if (const auto* arr_ptr = std::get_if<Array>(
		                       // std::get_if<std::vector<Value>>( // (***)
		                       &iterable_val.data)) {
			        // 対象が 配列 の場合 ([a, b, c] など)

			        for (const auto& elem : *arr_ptr) {
				        // auto previous_env = this->env;
				        try {
					        auto loop_res = this->execute_for_iteration(
					            s.loop_variable.lexeme, elem, s.body.get());
					        if (!loop_res) {
						        return loop_res;
					        }

				        } catch (const dara::backend::ContinueException&) {
					        // 何もせず環境を戻して、次のイテレーション（C++のループ）へ進む
					        // this->env = previous_env;
					        continue;
				        } catch (const dara::backend::BreakException&) {
					        // 環境を戻して、C++のループ自体を終了する
					        // this->env = previous_env;
					        break;
				        }
			        }

		        } else {
			        return std::unexpected(dara::error::InterpreterError{
			            "Object is not iterable."});
		        }

		        return {};
	        },
	        [this](const BlockStmt& s) -> Result<void> {
		        auto previous_env = this->env;

		        // this->env = std::make_shared<Environment>(previous_env);
		        // this->env = std::allocate_shared<Environment>(this->alloc,
		        // previous_env);
		        this->env = std::allocate_shared<Environment>(
		            this->alloc, this->alloc, previous_env);

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
		        return std::unexpected(dara::error::InterpreterError(
		            "unimplemented statement execution"));
	        }},
	    stmt->value);
};

Result<void> Interpreter::execute_for_iteration(const std::string& var_name,
                                                const Value& val, Stmt* body) {
	// auto loop_env = std::make_shared<Environment>(this->env);
	// auto loop_env = std::allocate_shared<Environment>(this->alloc,
	// this->env);
	auto loop_env =
	    std::allocate_shared<Environment>(this->alloc, this->alloc, this->env);
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

// helper function
int Interpreter::factorial(int n) {
	if (n <= 1) return 1;
	auto rng = std::views::iota(1, n + 1);
	return std::accumulate(rng.begin(), rng.end(), 1LL, std::multiplies<int>());
}

// helper function
static bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
                              std::shared_ptr<dara::backend::Class> target) {
	if (!current) return false;
	if (current == target) return true;
	for (const auto& mixin : current->mixins) {
		if (check_inheritance(mixin, target)) return true;
	}
	return check_inheritance(current->super, target);
}

/* eval_infix */
Result<Value> Interpreter::eval_infix(InfixOperator op, const Value& lhs_val,
                                      const Value& rhs_val, const Expr* expr) {
	return std::visit(
	    overloaded{
	        // for integer
	        /*
	        [op, expr](int l, int r) -> Result<Value> {
	            switch (op) {
	                case InfixOperator::Add:
	                    return Value{l + r};
	                case InfixOperator::Sub:
	                    return Value{l - r};
	                case InfixOperator::Mul:
	                    return Value{l * r};
	                case InfixOperator::Pow:
	                    return Value{static_cast<int>(std::pow(l, r))};
	                case InfixOperator::Div:
	                    if (r == 0) {
	                        return std::unexpected(
	                            dara::error::InterpreterError{"division by
	        zero"});
	                        //"division by zero", expr->line, expr->col});
	                    }
	                    return Value{l / r};
	                case InfixOperator::Greater:
	                    return Value{l > r};
	                case InfixOperator::GreaterEqual:
	                    return Value{l >= r};
	                case InfixOperator::Less:
	                    return Value{l < r};
	                case InfixOperator::LessEqual:
	                    return Value{l <= r};
	                case InfixOperator::EqualEqual:
	                    return Value{l == r};
	                case InfixOperator::NotEqual:
	                    return Value{l != r};
	                case InfixOperator::Range:
	                    if (typeid(l) == typeid(int) &&
	                        typeid(r) == typeid(int)) {
	                        return Value{dara::backend::Range{l, r}};
	                    } else {
	                        return
	        std::unexpected(dara::error::InterpreterError{ "Range operands must
	        be integers."});
	                    }
	                    // return Value{l != r};
	                default:
	                    return
	        std::unexpected(dara::error::InterpreterError{std::format(
	                        "[infix] unsupported operator error. {}",
	                        expr->to_string())});
	                    // expr->line, expr->col});
	            }
	        },
	        */
	        // Numeric
	        [op, expr, this](const Numeric auto& l,
	                         const Numeric auto& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        return Value{l + r};
				        // std::string l_str =
				        // dara::backend::to_string(Value{l}); String res(l_str,
				        // this->alloc); res += r; return Value{std::move(res)};
			        }
			        case InfixOperator::Sub:
				        return Value{l - r};
			        case InfixOperator::Mul:
				        return Value{l * r};
			        case InfixOperator::Pow:
				        // return Value{static_cast<int>(std::pow(l, r))};
				        if constexpr (std::is_same_v<std::decay_t<decltype(l)>,
				                                     int> &&
				                      std::is_same_v<std::decay_t<decltype(r)>,
				                                     int>) {
					        return Value{static_cast<int>(std::pow(l, r))};
				        } else {
					        return Value{static_cast<double>(std::pow(l, r))};
				        }
			        case InfixOperator::Div:
				        if (r == 0) {
					        return std::unexpected(
					            dara::error::InterpreterError{
					                "division by zero"});
				        }
				        return Value{l / r};
			        case InfixOperator::Greater:
				        return Value{l > r};
			        case InfixOperator::GreaterEqual:
				        return Value{l >= r};
			        case InfixOperator::Less:
				        return Value{l < r};
			        case InfixOperator::LessEqual:
				        return Value{l <= r};
			        case InfixOperator::EqualEqual:
				        return Value{l == r};
			        case InfixOperator::NotEqual:
				        return Value{l != r};
			        case InfixOperator::Range:
				        if constexpr (std::is_same_v<std::decay_t<decltype(l)>,
				                                     int> &&
				                      std::is_same_v<std::decay_t<decltype(r)>,
				                                     int>) {
					        return Value{dara::backend::Range{l, r}};
				        } else {
					        return std::unexpected(
					            dara::error::InterpreterError{
					                "Range operands must be integers."});
				        }
				        // return Value{l != r};
			        default:
				        return std::unexpected(
				            dara::error::InterpreterError{std::format(
				                "[infix] unsupported operator error. {}",
				                expr->to_string())});
				        // expr->line, expr->col});
		        }
	        },
	        // for string
	        //[op, expr, this](const std::string& l,
	        //                 const std::string& r) -> Result<Value> {
	        [op, expr, this](const String& l,
	                         const String& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        // return Value{l + r};
				        String res(l, this->alloc);
				        res += r;
				        return Value{std::move(res)};
			        }
			        case InfixOperator::EqualEqual:
				        return Value{l == r};
			        case InfixOperator::NotEqual:
				        return Value{l != r};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            "unsupported operator for strings"});
		        }
	        },
	        //[op, expr, this](const std::string& l,
	        [op, expr, this](const String& l,
	                         const Numeric auto& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        // return Value{l + std::to_string(r)};
				        // return Value{l + dara::backend::to_string(Value{r})};
				        // std::string l_str =
				        // dara::backend::to_string(Value{l});
				        String res(l, this->alloc);
				        res += dara::backend::to_string(Value{r});
				        return Value{std::move(res)};
			        }
			        case InfixOperator::EqualEqual:
			        case InfixOperator::StrictEqual:
				        return Value{false};
			        case InfixOperator::NotEqual:
				        return Value{true};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            "unsupported operator for string and integer"});
		        }
	        },
	        [op, expr, this](const Numeric auto& l,
	                         // const std::string& r) -> Result<Value> {
	                         const String& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        // return Value{std::to_string(l) + r};
				        // return Value{dara::backend::to_string(Value{l}) + r};
				        std::string l_str = dara::backend::to_string(Value{l});
				        String res(l_str, this->alloc);
				        res += r;
				        return Value{std::move(res)};
			        }
			        case InfixOperator::EqualEqual:
			        case InfixOperator::StrictEqual:
				        return Value{false};
			        case InfixOperator::NotEqual:
				        return Value{true};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            "unsupported operator for integer and string"});
		        }
	        },

	        /*
	        [op, expr](const std::string& l,
	                   const int r) -> Result<Value> {
	            switch (op) {
	                case InfixOperator::Add:
	                    return Value{l + std::to_string(r)};
	                case InfixOperator::EqualEqual:
	                case InfixOperator::StrictEqual:
	                    return Value{false};
	                case InfixOperator::NotEqual:
	                    return Value{true};
	                default:
	                    return std::unexpected(dara::error::InterpreterError{
	                        "unsupported operator for string and integer"});
	            }
	        },
	        [op, expr](const int l,
	                   const std::string& r) -> Result<Value> {
	            switch (op) {
	                case InfixOperator::Add:
	                    return Value{std::to_string(l)+r};
	                case InfixOperator::EqualEqual:
	                case InfixOperator::StrictEqual:
	                    return Value{false};
	                case InfixOperator::NotEqual:
	                    return Value{true};
	                default:
	                    return std::unexpected(dara::error::InterpreterError{
	                        "unsupported operator for integer and string"});
	            }
	        },
	        */
	        /*

	    /*
	    [op, expr](
	        std::shared_ptr<dara::backend::Instance> l,
	        std::shared_ptr<dara::backend::Class> r) ->
	    Result<Value> {
	        // return Value{.data = true};
	        if (op == InfixOperator::Is) {
	            return Value{.data =
	                                  check_inheritance(l->get_class(),
	    r)};
	        }
	        return std::unexpected(dara::error::InterpreterError{"unsupported
	    oerator"});
	    },
	    // (int, int) or (string, string) 以外 i.e. mismatch
	    [expr](const auto&, const auto&) -> Result<Value> {
	        return std::unexpected(
	            dara::error::InterpreterError{"type mismatch in binary
	    operation", expr->line, expr->col});
	    }},
	    */

	        [op](std::shared_ptr<dara::backend::Instance> l,
	             std::shared_ptr<dara::backend::Instance> r) -> Result<Value> {
		        // return Value{.data = true};
		        if (op == InfixOperator::EqualEqual) {
			        return Value{.data = (l == r)};
		        }
		        if (op == InfixOperator::StrictEqual) {
			        return Value{.data = (l == r)};
		        }
		        return std::unexpected(
		            dara::error::InterpreterError{"unsupported oerator"});
	        },
	        /* (***) */
	        [op, expr](
	            std::shared_ptr<dara::backend::Instance> l,
	            std::shared_ptr<dara::backend::Class> r) -> Result<Value> {
		        if (op == InfixOperator::Is) {
			        return Value{.data = check_inheritance(l->get_class(), r)};
		        }
		        if (op == InfixOperator::StrictEqual) {
			        return Value{.data = (l->get_class() == r)};
		        }
		        // return Value{.data = false};
		        return std::unexpected(
		            dara::error::InterpreterError{"unsupported operator"});
	        },

	        //[op, expr](const auto&, const auto& r_val) -> Result<Value> {
	        [op, expr](const auto&, const auto&) -> Result<Value> {
		        //
		        if (op == InfixOperator::Is) {
			        /*
			        using ClassPtr = std::shared_ptr<dara::backend::Class>;
			        if constexpr (!std::is_same_v<std::decay_t<decltype(r_val)>,
			                                      ClassPtr>) {
			            return std::unexpected(dara::error::InterpreterError{
			                "right operand of 'is' must be a class", expr->line,
			                expr->col});
			        }
			        return Value{.data = false};
			        */
			        return Value{.data = false};
		        }
		        if (op == InfixOperator::EqualEqual ||
		            op == InfixOperator::StrictEqual) {
			        return Value{.data = false};
		        }
		        if (op == InfixOperator::NotEqual) {
			        return Value{.data = true};
		        }
		        // return std::unexpected(dara::error::InterpreterError{
		        //     "type mismatch in binary operation", expr->line,
		        //     expr->col});
		        return std::unexpected(dara::error::InterpreterError{
		            "type mismatch in binary operation"});
	        }},
	    lhs_val.data, rhs_val.data);
}

/* eval_prefix */
Result<Value> Interpreter::eval_prefix(PrefixOperator op, const Value& rhs_val,
                                       const Expr* expr) {
	if (op == PrefixOperator::Not) {
		return Value{!is_truthy(rhs_val)};
	}

	return std::visit(
	    overloaded{[op, expr](const Numeric auto& r) -> Result<Value> {
		               switch (op) {
			               case PrefixOperator::Neg:
				               return Value{-r};
			               case PrefixOperator::Pos:
				               return Value{+r};
			               default:
				               return std::unexpected(
				                   dara::error::InterpreterError{
				                       "[ pre] unsupported operator error."});
				               //"[ pre] unsupported operator error.",
				               // expr->line, expr->col});
		               }
	               },
	               [expr](const auto&) -> Result<Value> {
		               return std::unexpected(dara::error::InterpreterError{
		                   "type mismatch in prefix operation"});
		               // dara::error::InterpreterError{"type mismatch in prefix
		               // operation", expr->line, expr->col});
	               }},
	    rhs_val.data);

	/*
	return std::visit(
	    overloaded{
	        [op, expr](int r) -> Result<Value> {
	            switch (op) {
	                case PrefixOperator::Neg:
	                    return Value{-r};
	                case PrefixOperator::Pos:
	                    return Value{+r};
	                default:
	                    return std::unexpected(dara::error::InterpreterError{
	                        "[ pre] unsupported operator error."});
	                    //"[ pre] unsupported operator error.",
	                    // expr->line, expr->col});
	            }
	        },
	        [expr](const auto&) -> Result<Value> {
	            return std::unexpected(
	                dara::error::InterpreterError{"type mismatch in prefix
	operation"});
	            // dara::error::InterpreterError{"type mismatch in prefix
	            // operation", expr->line, expr->col});
	        }

	    },
	    rhs_val.data);
	*/
}

/* eval_postfix */
Result<Value> Interpreter::eval_postfix(PostfixOperator op,
                                        const Value& lhs_val,
                                        const Expr* expr) {
	return std::visit(
	    overloaded{
	        [op, expr, this](int l) -> Result<Value> {
		        switch (op) {
				        /*
				       case PostfixOperator::Inc:
				           return Value{l};
				       case PostfixOperator::Dec:
				           return Value{l};
				        */
			        case PostfixOperator::Fac:
				        if (l < 0) {
					        return std::unexpected(
					            dara::error::InterpreterError{
					                .message = "factorial of negative number",
					                .span = expr->span,  //  短く書ける
					                //.actual =
					                //    std::nullopt  //  無理にcharにしない
					            });
					        // return std::unexpected(
					        //     dara::error::InterpreterError{
					        //         "factorial of negative number",
					        //         // expr->line, expr->col});
					        //         Span{expr->span.start, expr->span.end},
					        //         l + '0'});
				        }
				        return Value{this->factorial(l)};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            .message = "[post] unsupported operator error.",
				            .span = expr->span,
				            //.actual = std::nullopt
				        });

				        // return std::unexpected(dara::error::InterpreterError{
				        //     "[post] unsupported operator error.",
				        //     Span{expr->span.start, expr->span.end}, l +
				        //     '0'});
		        }
	        },
	        [expr](const auto& actual) -> Result<Value> {
		        return std::unexpected(dara::error::InterpreterError{
		            .message =
		                "type mismatch in postfix operation (expected integer)",
		            .span = expr->span,
		            //.actual = std::nullopt  // 🌟 ここもnulloptでOK！
		        });

		        // return std::unexpected(dara::error::InterpreterError{
		        //     "type mismatch in postfix operation",
		        //     Span{expr->span.start, expr->span.end}, actual});  //***
		        //  expr->line,
		        //   xpr->col});
	        }},
	    lhs_val.data);
}

/*
Result<Value> Interpreter::eval_mixfix(MixfixOperator op,
                                            const Value& lhs_val,
                                            const dara::Expr* expr) {
    //
    // Result<dara::CallExpr> callee_value = this->env->get(lhs_val.data);;
}
*/

Result<Value> Interpreter::visit_expr(const Expr* expr) {
	if (!expr) {
		// 🌟 エラーになったノードの start が何を持っているか確認！
		std::cout << "[DEBUG] Error Node Span Start: " << expr->span.start
		          << "[DEBUG] Error Node Span end: " << expr->span.end
		          << std::endl;
	}

	return std::visit(
	    overloaded{
	        [](const BoolExpr& e) -> Result<Value> { return Value{e.value}; },
	        [](const DoubleExpr& e) -> Result<Value> { return Value{e.value}; },
	        [](const IntExpr& e) -> Result<Value> { return Value{e.value}; },
	        [this](const ThisExpr& e) -> Result<Value> {
		        return this->env->get("this").value();
	        },

	        [this](const StringExpr& e) -> Result<Value> {
		        // return Value{e.value};
		        return Value{String(e.value.c_str(), this->alloc)};
	        },
	        [](const CharExpr& e) -> Result<Value> { return Value{e.value}; },
	        [this](const FunctionExpr& e) -> Result<Value> {
		        std::shared_ptr<Environment> current_env = this->env;
		        // auto function = std::make_shared<dara::backend::Function>(
		        auto function = std::allocate_shared<dara::backend::Function>(
		            this->alloc, &e, current_env);  //(*)

		        return Value{function};
	        },
	        /*
	        [this](const ArrayExpr& e) -> Result<Value> {
	            std::vector<Value> evaluated_elements;
	            for (const auto& elem : e.elements) {
	                auto evaluated_val = this->visit_expr(elem.get());
	                if (!evaluated_val) {
	                    return std::unexpected(evaluated_val.error());
	                }
	                evaluated_elements.push_back(evaluated_val.value());
	            }
	            return Value{evaluated_elements};
	        },
	        */
	        // PMR
	        [this, expr](const ArrayExpr& e) -> Result<Value> {
		        Array evaluated_elements(this->alloc);
		        for (const auto& elem : e.elements) {
			        auto evaluated_val = this->visit_expr(elem.get());
			        if (!evaluated_val) {
				        return std::unexpected(evaluated_val.error());
			        }
			        evaluated_elements.push_back(evaluated_val.value());
		        }
		        return Value{std::move(evaluated_elements)};
	        },
	        [this, expr](const IndexExpr& e) -> Result<Value> {
		        auto array_res = this->visit_expr(e.array.get());
		        if (!array_res) {
			        return std::unexpected(array_res.error());
		        }

		        auto index_res = this->visit_expr(e.index.get());
		        if (!index_res) {
			        return std::unexpected(index_res.error());
		        }

		        const auto* arr_ptr =
		            // std::get_if<std::vector<Value>>(&array_res.value().data);
		            std::get_if<Array>(&array_res.value().data);
		        if (!arr_ptr) {
			        return std::unexpected(dara::error::InterpreterError{
			            "Only arrays can be indexed", expr->span});
		        }

		        const auto* index_ptr =
		            std::get_if<int>(&index_res.value().data);
		        if (!index_ptr) {
			        return std::unexpected(dara::error::InterpreterError{
			            "Array index must be integer"});
		        }
		        int index = *index_ptr;
		        return (*arr_ptr)[index];
	        },

            /* next 0927 */
	        //[this, expr](const IdentifierExpr& e) -> Result<Value> {
	        [this, expr](const VarExpr& e) -> Result<Value> {
		        // auto val_opt = this->env.get(e.name);
		        auto val_opt = this->env.get()->get(e.name.lexeme);
		        if (!val_opt) {
			        PRINT_LINE();
			        return std::unexpected(dara::error::InterpreterError{
			            .message = "undefined variable '" + e.name.lexeme + "'",
			            .span = e.name.span,
			            //.span = expr->span,
			            //.actual = std::nullopt
			        });

			        // expr->line, expr->col});
		        }
		        // return val_opt.value();
		        return Value{val_opt.value()};
	        },
	        [this, expr](const CallExpr& e) -> Result<Value> {
		        // auto val_opt = this->env.get(e.name);
		        // auto val_opt = this->env->get(*e.callee);

		        /* (****) */
		        auto val_opt = this->visit_expr(e.callee.get());

		        if (!val_opt) {
			        // auto val_err = val_opt.error();
			        // val_err.span = expr->span;
			        return std::unexpected(val_opt.error());
		        }
		        Value callee = val_opt.value();

		        std::vector<Value> arguments;
		        // Array arguments;
		        for (const auto& arg_expr : e.arguments) {
			        auto arg_res = this->visit_expr(arg_expr.get());
			        if (!arg_res) return std::unexpected(arg_res.error());
			        arguments.push_back(arg_res.value());  //(A)
		        }

		        return std::visit(
		            overloaded{
		                /*
		                [&, expr](
		                    std::shared_ptr<dara::backend::Callable> callable)
		                    -> Result<Value> {
		                    if (arguments.size() != callable->arity()) {
		                        return std::unexpected(
		                            dara::error::InterpreterError{
		                                "Expected " +
		                                    std::to_string(callable->arity()) +
		                                    " argument but got " +
		                                    std::to_string(arguments.size()) +
		                                    ".",
		                                expr->span});
		                    }
		                    return callable->call(*this, arguments);
		                },
		                */
		                [&, expr](
		                    std::shared_ptr<dara::backend::Callable> callable)
		                    -> Result<Value> {
			                if (arguments.size() != callable->arity()) {
				                return std::unexpected(
				                    dara::error::InterpreterError{
				                        "Expected " +
				                            std::to_string(callable->arity()) +
				                            " argument but got " +
				                            std::to_string(arguments.size()) +
				                            ".",
				                        expr->span});
			                }
			                auto call_res = callable->call(*this, arguments);
			                if (!call_res) {
				                // auto call_res_err = call_res.error();
				                // call_res_err.span = expr->span;
				                return std::unexpected(call_res.error().enrich_span(expr->span));
			                }
			                return call_res;
		                },
		                /* A */
		                [&, expr](std::shared_ptr<dara::backend::Class> cls)
		                    -> Result<Value> {
			                if (arguments.size() != cls->arity()) {
				                return std::unexpected(
				                    dara::error::InterpreterError{
				                        "Expected " +
				                            std::to_string(cls->arity()) +
				                            " argument but got " +
				                            std::to_string(arguments.size()) +
				                            ".",
				                        expr->span});
			                }

			                auto call_res = cls->call(*this, arguments);
			                if (!call_res) {
				                //auto call_res_err = call_res.error();
				                //call_res_err.span = expr->span;
				                //return std::unexpected(call_res_err);
				                return std::unexpected(call_res.error().enrich_span(expr->span));
			                }
                            return call_res;
			                // return cls->call(*this, arguments);

			                /*
			                std::shared_ptr<dara::backend::Instance> instance
			                = cls->instantiate();

			                if (cls->methods.contains("new")) {
			                    auto init_res = instance->get("new");
			                    if (!init_res) {
			                        return
			                std::unexpected(init_res.error());
			                    }
			                    if (auto* callable_ptr =
			                            std::get_if<std::shared_ptr<
			                                dara::backend::Callable>>(
			                                &init_res.value().data)) {
			                        auto constructor = *callable_ptr;

			                        if (arguments.size() !=
			                            constructor->arity()) {
			                            return
			                std::unexpected(dara::error::InterpreterError{
			                "Expected " + std::to_string( constructor->arity())
			                + " argument but got " +
			                                std::to_string(arguments.size())
			                +
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
			                        return
			                std::unexpected(dara::error::InterpreterError(
			                            "Expected 0 arguments but got " +
			                            std::to_string(arguments.size())));
			                    }
			                }

			                return Value{instance};
			                */
		                },
		                [&](auto&&) -> Result<Value> {
			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "Can only call function and classes",
			                        expr->span));
		                }},
		            callee.data);
	        },
	        [this, expr](const GetExpr& e) -> Result<Value> {
		        // object.methodのうちまず"object"の方を評価する
		        auto obj_res = this->visit_expr(e.object.get());
		        if (!obj_res) {
			        return std::unexpected(obj_res.error());
		        }
		        // 左のnodeを作成終了
		        Value obj = obj_res.value();

		        // object.methodのうち"method"の方を評価する。実質は既に登録されていないか"探す
		        // nodeのdataが1. instance 2. class, 3. それ以外
		        // 見つかったらmethodの名前を使ってgetし、環境に登録していあるValueをそのまま帰す
		        return std::visit(
		            overloaded{
		                // 1.
		                // インスタンスに対するプロパティ/メソッドアクセス
		                [&](std::shared_ptr<dara::backend::Instance> instance)
		                    -> Result<Value> {
			                auto inst_res = instance->get(*this, e.name.lexeme);
			                if (!inst_res) {
				                //auto inst_err = inst_res.error();
				                //inst_err.span = expr->span;
				                //return std::unexpected(inst_err);
				                //return std::unexpected(inst_res.error().enrich_span(expr->span));
				                return std::unexpected(inst_res.error().enrich_span(e.name.span));
			                }
			                return inst_res;
		                },
		                //  2. クラスに対するスタティックメソッドアクセス
		                // (Person.new など)
		                [&](std::shared_ptr<dara::backend::Class> cls)
		                    -> Result<Value> {
			                auto cls_res = cls->get(*this, e.name.lexeme);
			                if (!cls_res) {
				                auto cls_err = cls_res.error();
				                cls_err.span = expr->span;
				                return std::unexpected(cls_err);
			                }
			                return cls_res;
		                },
		                // 3. それ以外の型にはドットアクセス不可
		                [&](auto&&) -> Result<Value> {
			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "only instances and classes have "
			                        "properties.",
			                        expr->span));
		                }},
		            obj.data);

		        /*
		        if (auto* instance =
		                std::get_if<std::shared_ptr<dara::backend::Instance>>(
		                    &obj.data)) {
		            return (*instance)->get(e.name);  //(*)
		        }
		        return std::unexpected(
		            dara::error::InterpreterError("only instances ave
		        properties"));
		        */
	        },

	        [this, expr](const InfixOpExpr& e) -> Result<Value> {
		        auto lhs_res = this->visit_expr(e.lhs.get());
		        if (!lhs_res) return std::unexpected(lhs_res.error());

		        auto rhs_res = this->visit_expr(e.rhs.get());
		        if (!rhs_res) return std::unexpected(rhs_res.error());

		        return this->eval_infix(e.op, lhs_res.value(), rhs_res.value(),
		                                expr);
	        },
	        [this, expr](const LogicalOpExpr& e) -> Result<Value> {
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
	        [this, expr](const PrefixOpExpr& e) -> Result<Value> {
		        auto rhs_res = this->visit_expr(e.rhs.get());
		        if (!rhs_res) return std::unexpected(rhs_res.error());

		        return this->eval_prefix(e.op, rhs_res.value(), expr);
	        },
	        [this, expr](const PostfixOpExpr& e) -> Result<Value> {
		        auto lhs_res = this->visit_expr(e.lhs.get());
		        if (!lhs_res) return std::unexpected(lhs_res.error());

		        return this->eval_postfix(e.op, lhs_res.value(), expr);
	        },

	        [expr](const auto&) -> Result<Value> {
		        PRINT_LINE();
		        // return std::unexpected(dara::error::InterpreterError{
		        //     "unimplemented expression evaluation", expr->line,
		        //     expr->col});
		        return std::unexpected(dara::error::InterpreterError{
		            .message = "unimplemented expression evaluation",
		            .span = expr->span,
		            //.actual = std::nullopt
		        });
	        },
	        /*
	        [expr](const auto&) -> Result<Value> {
	            PRINT_LINE();
	            return std::unexpected(
	                dara::error::InterpreterError{"unimplemented expression
	        evaluation", expr->line, expr->col});
	            // dara::error::InterpreterError {"unimplemented expression
	            // evaluation"});::
	        },
	        */
	    },
	    expr->value);
}

}  // namespace dara::backend

/*
int Evaluator::visit_expr(const dara::Expr* expr) {  // 実装は .cpp へ
    return std::visit(
        overloaded{[](const dara::IntExpr& expr) { return expr.value; },

                   [this](const dara::InfixOpExpr& expr) {
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
                   [this](const dara::PrefixOpExpr& expr) {
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

                   [this](const dara::PostfixOpExpr& expr) {
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
