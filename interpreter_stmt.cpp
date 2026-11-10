
/* interpreter_decl.cpp */
//#pragma once

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
#include "array.hpp"
#include "builtin.hpp"
#include "environment.hpp"
#include "map.hpp"
#include "parser.hpp"
#include "string.hpp"

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

/* visit_stmt in visit_stmt.cpp  */
// __visit_stmt__TOP
Result<void> Interpreter::visit_stmt(const Stmt* stmt) {
	return std::visit(
	    overloaded{
	        /*
	                    [this](const dara::ReturnStmt& s) ->
	           Result<void> { Value ret_val; ret_val.data =
	           std::monostate{};

	                        if (s.value != nullptr) {
	                            auto res =
	           this->visit_expr(s.value.get()); if (!res) { return
	           std::unexpected(res.error());
	                            }
	                            ret_val = res.value();
	                        }
	                        return {};
	        */
	        // Interpreter::visit_stmt の中の ReturnStmt 処理のイメージ

	        // __visit_stmt__return_stmt
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
	        // __SetStmt__ //
	        // __visit_stmt__set_stmt__
	        [&, stmt](const SetStmt& set_stmt) -> Result<void> {
		        auto obj_res = this->visit_expr(set_stmt.object.get());
		        if (!obj_res) {
			        auto obj_err = obj_res.error();
			        // obj_err.span = stmt->span;
			        // return std::unexpected(obj_err);
			        return std::unexpected(
			            obj_res.error().enrich_span(stmt->span));
		        }
		        // return obj_res.error();

		        Value obj = obj_res.value();
		        const std::string& prop_name = set_stmt.name.lexeme;

		        if (auto* instance = std::get_if<
		                std::shared_ptr<dara::backend::Instance>>(  // D
		                &obj.data)) {
			        if (auto cls = (*instance)->get_class()) {
				        // const std::string prop_name =
				        // set_stmt.name.lexeme; const auto& prop_name =
				        // String(set_stmt.name.lexeme, alloc);
				        // bool is_field = cls && cls->has_field(prop_name);
				        // bool is_method = cls && cls->has_method(prop_name);

				        // if (cls && !cls->has_field(set_stmt.name.lexeme)
				        // && !cls->has_method(set_stmt.name.lexeme)) {
				        // if (cls && !is_field && !is_method) {
				        // if (cls && !is_field) {
				        // if (is_method) {
				        if (auto valid_res =
				                cls->validate_field_assign(prop_name);
				            !valid_res) {
					        return std::unexpected(
					            valid_res.error().enrich_span(
					                set_stmt.name.span));
					        /*
					        dara::error::InterpreterError{
					            .message = "cannot assign to method'" +
					                       // set_stmt.name.lexeme + "' on
					                       // class '" +
					                       prop_name + "' on class '" +
					                       cls->name + "'",
					            .span = set_stmt.name.span});
					        */
				        }
				        // return std::unexpected(dara::error::InterpreterError{
				        //     .message =
				        //         "underdined field or method'" +
				        //         // set_stmt.name.lexeme + "' on class '" +
				        //         prop_name + "' on class '" + cls->name + "'",
				        //     .span = set_stmt.name.span});
				        // }
			        }

			        auto val_res = this->visit_expr(set_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }

			        // Value val = val_res.value();

			        // (san error)
			        /*
			          if (is_method) {
			              if (auto* callable_ptr = std::get_if<
			                      std::shared_ptr<dara::backend::Callable>>(
			                      &val.data)) {
			                  if (auto fn = std::dynamic_pointer_cast<
			                          dara::backend::Function>(*callable_ptr))
			          { auto bound_fn = fn->bind(*this, *instance); val
			          = Value{ std::static_pointer_cast<
			                              dara::backend::Callable>(bound_fn)};
			                  }
			              }
			          }
			          */
			        // san error this biding

			        //(*instance)->set(set_stmt.name.lexeme, val_res.value());
			        (*instance)->set(prop_name, val_res.value());
			        return {};

			        // __visit_stmt__set_stmt__set_static_field__
		        } else if (auto* cls_ptr = std::get_if<
		                       std::shared_ptr<dara::backend::Class>>(
		                       &obj.data)) {
			        // __set_stmt__set_static_let__
			        auto& cls = *cls_ptr;
			        /*
			        if (!cls->has_static_field(prop_name)) {
			            return std::unexpected(dara::error::InterpreterError{
			                .message = "cannot assign tot static method '" +
			                           prop_name + "' on class '" + cls->name +
			                           "'",
			                .span = set_stmt.name.span});
			        }
			        */
                    /*
			        if (auto valid_res =
			                cls->validate_static_field_assign(prop_name);
			            !valid_res) {
				        return std::unexpected(
				            valid_res.error().enrich_span(set_stmt.name.span));
			        }
                    */
			        /*
			        return std::unexpected(dara::error::InterpreterError{
			            .message = "undefined static field '" + prop_name +
			                       "' on class '" + cls->name + "'",
			            .span = set_stmt.name.span});
			        */

			        if (auto valid_res =
			                cls->validate_static_field_assign(prop_name);
			            !valid_res) {
				        return std::unexpected(
				            valid_res.error().enrich_span(set_stmt.name.span));
			        }
			        auto val_res = this->visit_expr(set_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }
			        cls->set_static(prop_name, this->ensure_value_semantics(
			                                       val_res.value()));
                    return {};
		        }
		        return std::unexpected(dara::error::InterpreterError(
		            "only instances have fields", stmt->span));
	        },
	        // __visit_stmt__break_stmt__
	        [&](const BreakStmt& stmt) -> Result<void> {
		        throw dara::backend::BreakException{};
	        },
	        // __visit_stmt__continue_stmt__
	        [&](const ContinueStmt& stmt) -> Result<void> {
		        throw dara::backend::ContinueException{};
	        },

	        // __visit_stmt__expr_stmt__
	        [this](const ExprStmt& s) -> Result<void> {
		        auto res = this->visit_expr(s.expr.get());
		        if (!res) return std::unexpected(res.error());
		        return {/* next is here 612 */};
	        },
	        // __visit_stmt__print_stmt__
	        [this, stmt](const PrintStmt& print_stmt) -> Result<void> {
		        auto res = this->visit_expr(print_stmt.expr.get());
		        if (!res) {
			        return std::unexpected(res.error().enrich_span(stmt->span));
			        // return
			        // std::unexpected(res.error().enrich_span(stmt->span);
			        // return
			        // std::unexpected(dara::error::InterpreterError(
			        //"only instance have fields", stmt->span));
		        }
		        this->out << dara::backend::to_string(res.value())
		                  << "\n";  //(**)
		        return {};
	        },
	        // __visit_stmt__inc_stmt__
	        [this](const IncStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name.lexeme);
		        if (!val_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(dara::error::InterpreterError{
			            "Operand '" + stmt.name.lexeme +
			            "' must be a number."});

		        auto assign_res =
		            this->env->assign(stmt.name.lexeme, Value{*num + 1});
		        if (!assign_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        return {};
	        },
	        // __visit_stmt__dec_stmt__
	        [this](const DecStmt& stmt) -> Result<void> {
		        auto val_res = this->env->get(stmt.name.lexeme);
		        if (!val_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        auto* num = std::get_if<int>(&val_res.value().data);
		        if (!num)
			        return std::unexpected(dara::error::InterpreterError{
			            "Operand '" + stmt.name.lexeme +
			            "' must be a number."});

		        auto assign_res =
		            this->env.get()->assign(stmt.name.lexeme, Value{*num - 1});
		        if (!assign_res)
			        return std::unexpected(dara::error::InterpreterError{
			            "undefined variable '" + stmt.name.lexeme + "'"});
		        return {};
	        },

	        // __visit_stmt__assign_stmt__
	        [this](const AssignStmt& s) -> Result<void> {
		        auto new_value_res = this->visit_expr(s.value.get());
		        if (!new_value_res) {
			        // PRINT_LINE();
			        return std::unexpected(new_value_res.error());
		        }

		        // Value new_value = new_value_res.value(); // old code
		        Value new_value = this->ensure_value_semantics(
		            new_value_res.value());  // new_code (value_semantics)

		        // auto success = this->env.assign(s.name, new_value);
		        auto success =
		            this->env.get()->assign(s.name.lexeme, new_value);
		        if (!success) {
			        // PRINT_LINE();
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
	        current_val.value(), val_res.value(), stmt.value.get());
	        Value new_val = eval_res.value();
	                    (*instance)->set(stmt.name, new_val);
	                }
	                return {};
	            }
	            return std::unexpected(
	                dara::error::InterpreterError("only instances have
	        fields"));
	        },
	        */
	        // __visit_stmt__conpound_assign_stmt__
	        [this](const CompoundAssignStmt& cas) -> Result<void> {
		        auto current_value_opt = this->env->get(cas.name.lexeme);  //(*)
		        if (!current_value_opt.has_value()) {
			        // PRINT_LINE();
			        return std::unexpected(
			            dara::error::InterpreterError(std::format(
			                "Underfined variable '{}'", cas.name.lexeme)));
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
	        /*
	        // __visit_stmt__conpound_set_stmt__
	        [this, stmt](const CompoundSetStmt& c_stmt) -> Result<void> {
	            // __visit_stmt__eval_object__
	            auto obj_res = this->visit_expr(c_stmt.object.get());
	            if (!obj_res) {
	                return std::unexpected(obj_res.error());
	            }
	            Value obj = obj_res.value();


	            // __visit_stmt__set_instance__
	            if (auto* instance =
	                    std::get_if<std::shared_ptr<dara::backend::Instance>>(
	                        &obj.data)) {
	                // current property value
	                auto current_val_opt =
	                    (*instance)->get(*this, c_stmt.name.lexeme);
	                if (!current_val_opt) {
	                    // auto val_opt_err = current_val_opt.error();
	                    // val_opt_err.span = stmt->span;
	                    // return std::unexpected(val_opt_err);
	                    // return
	                    // std::unexpected(val_opt_err.enrich_span(stmt->span));
	                    // //(A)
	                    return std::unexpected(
	                        current_val_opt.error().enrich_span(
	                            c_stmt.name.span));  //(A)

	                    // return
	                    // std::unexpected(dara::error::InterpreterError(
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
	                // auto set_res = (*instance)->set(c_stmt.name,
	                // new_val);

	                return {};
	            }
	            return std::unexpected(dara::error::InterpreterError(
	                "only instance have fields", stmt->span));
	        },
	        */
	        // __visit_stmt__conpound_set_stmt__
	        [this, stmt](const CompoundSetStmt& c_stmt) -> Result<void> {
		        // __visit_stmt__eval_object__
		        auto obj_res = this->visit_expr(c_stmt.object.get());
		        if (!obj_res) {
			        return std::unexpected(obj_res.error());
		        }
		        Value obj = obj_res.value();
		        const std::string& prop_name = c_stmt.name.lexeme;
		        // __visit_stmt__set_instance__
		        //
		        //

		        // __visit_stmt__set_instance__
		        if (auto* instance =
		                std::get_if<std::shared_ptr<dara::backend::Instance>>(
		                    &obj.data)) {
			        //__visit_stmt__check_field_exist__
			        if (auto cls = (*instance)->get_class()) {
				        if (auto valid_res =
				                cls->validate_field_assign(prop_name);
				            !valid_res) {
					        return std::unexpected(
					            valid_res.error().enrich_span(
					                c_stmt.name.span));
					        /*
					        auto cls = (*instance)->get_class();
					        // current property value



					        if (cls && !cls->has_field(prop_name)) {
					            if (cls->has_method(prop_name)) {
					                return std::unexpected(
					                    dara::error::InterpreterError{
					                        .message = "cannnot assign to method
					        '"
					        + prop_name + "' on class '" + cls->name + "'",
					        .span = c_stmt.name.span});
					            }
					            return
					        std::unexpected(dara::error::InterpreterError{
					        .message = "undefiend field '" + prop_name +
					                           "' on class '" + cls->name + "'",
					                .span = c_stmt.name.span});
					        }
					        */
				        }
			        }
			        // __visit_stmt__set_property_value
			        auto current_val_opt =
			            (*instance)->get(*this, c_stmt.name.lexeme);
			        if (!current_val_opt) {
				        return std::unexpected(
				            current_val_opt.error().enrich_span(
				                c_stmt.name.span));
			        }
			        Value current_val = current_val_opt.value();

			        auto val_res = this->visit_expr(c_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }
			        Value val = val_res.value();

			        auto eval_res = this->eval_infix(c_stmt.op, current_val,
			                                         val, c_stmt.value.get());
			        if (!eval_res) {
				        return std::unexpected(eval_res.error());
			        }
			        Value new_val = eval_res.value();
			        (*instance)->set(c_stmt.name.lexeme, new_val);

			        return {};

			        // __visit_stmt__set_static__
		        } else if (auto* cls_ptr = std::get_if<
		                       std::shared_ptr<dara::backend::Class>>(
		                       &obj.data)) {
			        auto& cls = *cls_ptr;

			        if (auto valid_res =
			                cls->validate_static_field_assign(prop_name);
			            !valid_res) {
				        return std::unexpected(
				            valid_res.error().enrich_span(c_stmt.name.span));
			        }
			        auto current_val_opt = cls->get(*this, prop_name);
			        if (!current_val_opt) {
				        return std::unexpected(
				            current_val_opt.error().enrich_span(
				                c_stmt.name.span));
			        }

			        Value current_val = current_val_opt.value();
			        auto val_res = this->visit_expr(c_stmt.value.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }

			        auto eval_res =
			            this->eval_infix(c_stmt.op, current_val,
			                             val_res.value(), c_stmt.value.get());
			        if (!eval_res) {
				        return std::unexpected(eval_res.error());
			        }

			        cls->set_static(prop_name, this->ensure_value_semantics(
			                                       eval_res.value()));
			        //__visit_stmt__check_field_exist__
			        // if (!cls->has_static_field(prp_name)) {
			        //    if (cls->has_static_method(prop_name))
			        //}
			        return {};
		        }

		        return std::unexpected(dara::error::InterpreterError(
		            "only instance have fields", stmt->span));
	        },

	        // __visit_stmt__if_stmt__
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
	        // __visit_stmt__while_stmt__
	        [this](const WhileStmt& stmt) -> Result<void> {
		        while (true) {
			        auto cond_res = this->visit_expr(stmt.condition.get());
			        if (!cond_res) return std::unexpected(cond_res.error());

			        if (!Interpreter::is_truthy(cond_res.value())) {
				        break;
			        }

			        // auto body_res =
			        // this->visit_stmt(stmt.body.get()); if (!body_res)
			        // return std::unexpected(body_res.error());
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
	        // __visit_stmt__forin_stmt__
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
			        //} else if (const auto* arr_ptr =
			        // std::get_if<Array>(&iterable_val.data)) {
		        } else if (const auto* arr_ptr =
		                       std::get_if<std::shared_ptr<Array>>(
		                           &iterable_val.data)) {  //(****)
			        // using Array = std::pmr::vector<Value>;
			        // std::shared_ptr<Array>
			        //  std::get_if<std::vector<Value>>( // (***)
			        // if (const auto* val_ptr = std::get_if<arr_ptr>) {
			        if (*arr_ptr != nullptr) {
				        // 対象が 配列 の場合 ([a, b, c] など)

				        Array* raw_array = (*arr_ptr).get();
				        for (const auto& elem : *raw_array) {
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
			        }
		        } else {
			        return std::unexpected(dara::error::InterpreterError{
			            "Object is not iterable."});
		        }

		        return {};
	        },
	        // __visit_stmt__block_stmt__
	        [this](const BlockStmt& s) -> Result<void> {
		        auto previous_env = this->env;

		        // this->env =
		        // std::make_shared<Environment>(previous_env);
		        // this->env =
		        // std::allocate_shared<Environment>(this->alloc,
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

}
