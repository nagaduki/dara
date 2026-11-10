/* interpreter.cpp */

// #pragma once
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#include "interpreter.hpp"
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

template <typename T>
concept Numeric = std::is_same_v<T, int> || std::is_same_v<T, double>;

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
	        // Numeric
	        [op, expr, this](const Numeric auto& l,
	                         const Numeric auto& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        return Value{l + r};
			        }
			        case InfixOperator::Sub:
				        return Value{l - r};
			        case InfixOperator::Mul:
				        return Value{l * r};
			        case InfixOperator::Pow:
				        // return Value{static_cast<int>(std::pow(l,
				        // r))};
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
					                "Range operands must be "
					                "integers."});
				        }
				        // return Value{l != r};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            std::format("unsupported operator error. {}",
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
	        // 1026
	        /*
	        [op, expr, this](const String& l,
	                         const Numeric auto& r) -> Result<Value> {
	            switch (op) {
	                case InfixOperator::Add: {
	                    // return Value{l + std::to_string(r)};
	                    // return Value{l +
	        dara::backend::to_string(Value{r})};
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
	                    return
	        std::unexpected(dara::error::InterpreterError{ "unsupported
	        operator for string and integer"});
	            }
	        },
	        [op, expr, this](const Numeric auto& l,
	                         // const std::string& r) -> Result<Value> {
	                         const String& r) -> Result<Value> {
	            switch (op) {
	                case InfixOperator::Add: {
	                    // return Value{std::to_string(l) + r};
	                    // return
	        Value{dara::backend::to_string(Value{l}) + r}; std::string
	        l_str = dara::backend::to_string(Value{l}); String
	        res(l_str, this->alloc); res += r; return
	        Value{std::move(res)};
	                }
	                case InfixOperator::EqualEqual:
	                case InfixOperator::StrictEqual:
	                    return Value{false};
	                case InfixOperator::NotEqual:
	                    return Value{true};
	                default:
	                    return
	        std::unexpected(dara::error::InterpreterError{ "unsupported
	        operator for integer and string"});
	            }
	        },
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

	        // char + any //
	        [op, this](const char l, const char r) -> Result<Value> {
		        if (op == InfixOperator::Add) {
			        std::string r_str = dara::backend::to_string(Value{r});
			        std::string l_str = dara::backend::to_string(Value{l});
			        // String str_l = std::basic_string(l, this->alloc);
			        String res(l_str + r_str, this->alloc);
			        // res += l;
			        return Value{std::move(res)};
		        }
		        return std::unexpected(dara::error::InterpreterError{
		            "unsupported oprator",
		        });
	        },
	        [op, expr, this](const char l,
	                         const Numeric auto& r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        std::string l_str = dara::backend::to_string(Value{l});
				        // return Value{l + std::to_string(r)};
				        // return Value{l +
				        // dara::backend::to_string(Value{r})};
				        // std::string l_str =
				        // dara::backend::to_string(Value{l});
				        String res(l_str, this->alloc);
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
				            "unsupported operator for string and "
				            "integer"});
		        }
	        },
	        [op, expr, this](const Numeric auto& l,
	                         // const std::string& r) -> Result<Value> {
	                         const char r) -> Result<Value> {
		        switch (op) {
			        case InfixOperator::Add: {
				        std::string r_str = dara::backend::to_string(Value{r});
				        // return Value{std::to_string(l) + r};
				        // return
				        // Value{dara::backend::to_string(Value{l}) +
				        // r};
				        std::string l_str = dara::backend::to_string(Value{l});
				        String res(l_str, this->alloc);
				        res += r_str;
				        return Value{std::move(res)};
			        }
			        case InfixOperator::EqualEqual:
			        case InfixOperator::StrictEqual:
				        return Value{false};
			        case InfixOperator::NotEqual:
				        return Value{true};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            "unsupported operator for integer and "
				            "string"});
		        }
	        },

	        /*
	        [op, this](const char l, const auto& r) -> Result<Value> {
	            if (op == InfixOperator::Add) {
	                std::string r_str =
	        dara::backend::to_string(Value{r});
	                // String str_l = std::basic_string(l, this->alloc);
	                String res(r_str, this->alloc);
	                res += l;
	                return Value{std::move(res)};
	            }
	            return std::unexpected(dara::error::InterpreterError{
	                "unsupported oprator",
	            });
	        },
	        [op, this](const auto& l, const char r) -> Result<Value> {
	            if (op == InfixOperator::Add) {
	                std::string l_str =
	        dara::backend::to_string(Value{l});
	                // String str_l = std::basic_string(l, this->alloc);
	                String res(l_str, this->alloc);
	                res += r;
	                return Value{std::move(res)};
	            }
	            return std::unexpected(dara::error::InterpreterError{
	                "unsupported oprator",
	            });
	        },
	        */

	        [op, this](const String& l, const auto& r) -> Result<Value> {
		        if (op == InfixOperator::Add) {
			        std::string r_str = dara::backend::to_string(Value{r});
			        String res(l, this->alloc);
			        res += r_str;
			        return Value{std::move(res)};
		        }
		        return std::unexpected(dara::error::InterpreterError{
		            "unsupported oprator",
		        });
	        },

	        [op, this](const auto& l, const String& r) -> Result<Value> {
		        if (op == InfixOperator::Add) {
			        std::string l_str = dara::backend::to_string(Value{l});
			        String res(l_str, this->alloc);
			        res += r;
			        return Value{std::move(res)};
		        }
		        return std::unexpected(dara::error::InterpreterError{
		            "unsupported oprator",
		        });
	        },
	        //[op, expr](const auto&, const auto& r_val) ->
	        // Result<Value> {
	        [op, expr](const auto&, const auto&) -> Result<Value> {
		        //
		        if (op == InfixOperator::Is) {
			        /*
			        using ClassPtr =
			        std::shared_ptr<dara::backend::Class>; if constexpr
			        (!std::is_same_v<std::decay_t<decltype(r_val)>,
			                                      ClassPtr>) {
			            return
			        std::unexpected(dara::error::InterpreterError{
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
		               // dara::error::InterpreterError{"type mismatch in
		               // prefix operation", expr->line, expr->col});
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
	                    return
	std::unexpected(dara::error::InterpreterError{
	                        "[ pre] unsupported operator error."});
	                    //"[ pre] unsupported operator error.",
	                    // expr->line, expr->col});
	            }
	        },
	        [expr](const auto&) -> Result<Value> {
	            return std::unexpected(
	                dara::error::InterpreterError{"type mismatch in
	prefix operation"});
	            // dara::error::InterpreterError{"type mismatch in
	prefix
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
					                //    std::nullopt  //
					                //    無理にcharにしない
					            });
					        // return std::unexpected(
					        //     dara::error::InterpreterError{
					        //         "factorial of negative number",
					        //         // expr->line, expr->col});
					        //         Span{expr->span.start,
					        //         expr->span.end}, l + '0'});
				        }
				        return Value{this->factorial(l)};
			        default:
				        return std::unexpected(dara::error::InterpreterError{
				            .message = "[post] unsupported "
				                       "operator error.",
				            .span = expr->span,
				            //.actual = std::nullopt
				        });

				        // return
				        // std::unexpected(dara::error::InterpreterError{
				        //     "[post] unsupported operator error.",
				        //     Span{expr->span.start, expr->span.end}, l
				        //     + '0'});
		        }
	        },
	        [expr](const auto& actual) -> Result<Value> {
		        return std::unexpected(dara::error::InterpreterError{
		            .message = "type mismatch in postfix operation "
		                       "(expected integer)",
		            .span = expr->span,
		            //.actual = std::nullopt  // 🌟 ここもnulloptでOK！
		        });

		        // return std::unexpected(dara::error::InterpreterError{
		        //     "type mismatch in postfix operation",
		        //     Span{expr->span.start, expr->span.end}, actual});
		        //     //***
		        //  expr->line,
		        //   xpr->col});
	        }},
	    lhs_val.data);
}

// __visit_expr__
Result<Value> Interpreter::visit_expr(const Expr* expr) {
	if (!expr) {
		//  エラーになったノードの start が何を持っているか確認！
		std::cout << "[DEBUG] Error Node Span Start: " << expr->span.start
		          << "[DEBUG] Error Node Span end: " << expr->span.end
		          << std::endl;
	}

	return std::visit(
	    overloaded{
	        [](const BoolExpr& e) -> Result<Value> { return Value{e.value}; },
	        [](const DoubleExpr& e) -> Result<Value> { return Value{e.value}; },
	        [](const IntExpr& e) -> Result<Value> { return Value{e.value}; },
	        /*
	        [this](const ThisExpr& e) -> Result<Value> {
	            return this->env->get("this").value();
	        },
	        */
	        [this, expr](const ThisExpr& e) -> Result<Value> {
		        auto this_opt = this->env->get("this");
		        if (!this_opt.has_value()) {
			        return std::unexpected(dara::error::InterpreterError{
			            .message = "cannot use 'this' outside of a method",
			            .span = expr->span});
		        }
		        return this->env->get("this").value();
	        },

	        [this](const StringExpr& e) -> Result<Value> {
		        // return Value{e.value};
		        return Value{String(e.value.c_str(), this->alloc)};
	        },
	        [](const CharExpr& e) -> Result<Value> { return Value{e.value}; },
	        [this](const FunctionExpr& e) -> Result<Value> {
		        std::shared_ptr<Environment> current_env = this->env;
		        // auto function =
		        // std::make_shared<dara::backend::Function>(
		        auto function = std::allocate_shared<dara::backend::Function>(
		            this->alloc, &e, current_env);  //(*)

		        return Value{function};
	        },
	        /*
	        [this](const ArrayExpr &e) -> Result<Value> {
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
		        auto array_ptr = std::allocate_shared<Array>(
		            this->alloc, std::move(evaluated_elements));

		        return Value{std::move(array_ptr)};
		        /*
		        std::shared_ptr<Array> evaluated_elements2(this->alloc);
		        for (const auto& elem : e.elements) {
		            auto evaluated_elements = evaluated_elements2.get();

		            auto evaluated_val = this->visit_expr(elem.get());
		            if (!evaluated_val) {
		                return std::unexpected(evaluated_val.error());
		            }

		            evaluated_elements.push_back(evaluated_val.value());
		        }

		        return Value{std::move(evaluated_elements2)};  //(A)
		                                                      // return
		        //
		        Value{std::make_shared<std::move(evaluated_elements)>};
		        // return Value{std::make_shared<evaluated_elements>};
		        // 1007
		        */
	        },
	        [this, expr](const TupleExpr& tpl_expr) -> Result<Value> {
		        std::pmr::vector<Value> elements(this->alloc);
		        elements.reserve(tpl_expr.elements.size());

		        for (const auto& elem_expr : tpl_expr.elements) {
			        auto val_res = this->visit_expr(elem_expr.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }
			        Value cloned_val =
			            this->ensure_value_semantics(val_res.value());
			        elements.push_back(std::move(cloned_val));
		        }

		        auto tpl_ptr = std::
allocate_shared
<dara::backend::Tuple>(
		            this->alloc, std::move(elements));

		        return Value{.data = std::move(tpl_ptr)};
	        },
	        [this, expr](const MapExpr& e) -> Result<Value> {
		        // auto map_obj = std::allocate_shared<Map>(this->alloc,
		        // this->alloc);

		        auto map_obj = std::allocate_shared<Map>(this->alloc);

		        for (const auto& pair : e.elements) {
			        auto val_res = this->visit_expr(pair.second.get());
			        if (!val_res) {
				        return std::unexpected(val_res.error());
			        }
			        std::pmr::string pmr_key(pair.first.c_str(), this->alloc);
			        (*map_obj)[pmr_key] = val_res.value();
		        }

		        return Value{.data = map_obj};
	        },
	        // visit_expr
	        [this, expr](const IndexExpr& e) -> Result<Value> {
		        auto target_res = this->visit_expr(e.target.get());
		        if (!target_res) {
			        return std::unexpected(target_res.error());
		        }
		        auto index_res = this->visit_expr(e.index.get());
		        if (!index_res) {
			        return std::unexpected(index_res.error());
		        }

		        const Value& target_val = target_res.value();
		        const Value& index_val = index_res.value();

		        if (auto* arr_ptr =
		                std::get_if<std::shared_ptr<Array>>(&target_val.data)) {
			        //
			        Array* raw_array = (*arr_ptr).get();
			        const auto* index_ptr =
			            std::get_if<int>(&index_res.value().data);
			        if (!index_ptr) {
				        return std::unexpected(dara::error::InterpreterError{
				            "Array index must be integer"});
			        }
			        int index = *index_ptr;
			        int arr_size = static_cast<int>(raw_array->size());

			        if (index < 0 || index >= arr_size) {
				        return std::unexpected(dara::error::InterpreterError{
				            "Array index out of bounds: " +
				                std::to_string(index) + " (size is " +
				                std::to_string(arr_size) + ")",
				            expr->span});
			        }
			        return (*raw_array)[index];
		        }
		        if (auto* map_ptr =
		                std::get_if<std::shared_ptr<Map>>(&target_val.data)) {
			        const auto* key_str = std::get_if<String>(&index_val.data);
			        if (!key_str) {
				        return std::unexpected(dara::error::InterpreterError(
				            "Map keys must be strings"));
			        }
			        auto map_obj = *map_ptr;
			        auto it = map_obj->find(*key_str);
			        if (it != map_obj->end()) {
				        return it->second;
			        } else {
				        return std::unexpected(dara::error::InterpreterError(
				            "key '" + std::string(key_str->c_str()) +
				            "' not found"));
			        }
		        }

		        return std::unexpected(dara::error::InterpreterError(
		            "only arrays and maps can be indexed", expr->span));
	        },
	        /*
	        [this, expr](const IndexExpr& e) -> Result<Value> {
	            // auto array_res = this->visit_expr(e.array.get());
	            //  target check
	            auto target_res = this->visit_expr(e.target.get());
	            if (!target_res) {
	                return std::unexpected(target_res.error());
	            }

	            // index check
	            auto index_res = this->visit_expr(e.index.get());
	            if (!index_res) {
	                return std::unexpected(index_res.error());
	            }

	            const Value& target_val = target_res.value();
	            const Value& index_val = index_res.value();

	            // if array
	            const auto* arr_ptr =
	                //
	        std::get_if<std::vector<Value>>(&array_res.value().data);
	                std::get_if<std::shared_ptr<Array>>(
	                    &target_res.value().data);
	            if (!arr_ptr) {
	                return
	        std::unexpected(dara::error::InterpreterError{ "Only arrays
	        can be indexed", expr->span});
	            }

	            // if map
	            const auto* map_ptr =
	                //
	        std::get_if<std::vector<Value>>(&array_res.value().data);
	                std::get_if<std::shared_ptr<Map>>(&target_res.value().data);
	            if (!map_ptr) {
	                return
	        std::unexpected(dara::error::InterpreterError{ "Only maps
	        can be indexed", expr->span});
	            }

	            if (*arr_ptr != nullptr) {
	                Array* raw_array = (*arr_ptr).get();
	                const auto* index_ptr =
	                    std::get_if<int>(&index_res.value().data);
	                if (!index_ptr) {
	                    return
	        std::unexpected(dara::error::InterpreterError{ "Array index
	        must be integer"});
	                }
	                int index = *index_ptr;
	                int arr_size = static_cast<int>(raw_array->size());

	                if (index < 0 || index >= arr_size) {
	                    return
	        std::unexpected(dara::error::InterpreterError{ "Array index
	        out of bounds: " + std::to_string(index) + " (size is " +
	                            std::to_string(arr_size) + ")",
	                        expr->span});
	                }
	                // return (*arr_ptr)[index];
	                return (*raw_array)[index];
	            } else {
	                return std::unexpected(
	                    dara::error::InterpreterError{"Array is Nil."});
	            }
	        },
	        */
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
		                    std::shared_ptr<dara::backend::Callable>
		                callable)
		                    -> Result<Value> {
		                    if (arguments.size() != callable->arity()) {
		                        return std::unexpected(
		                            dara::error::InterpreterError{
		                                "Expected " +
		                                    std::to_string(callable->arity())
		                + " argument but got " +
		                                    std::to_string(arguments.size())
		                +
		                                    ".",
		                                expr->span});
		                    }
		                    return callable->call(*this, arguments);
		                },
		                */
		                [&, expr](
		                    std::shared_ptr<dara::backend::Callable> callable)
		                    -> Result<Value> {
			                // if (callable->arity() != -1) {
			                if (arguments.size() != callable->arity() &&
			                    callable->arity() != -1) {
				                return std::unexpected(
				                    dara::error::InterpreterError{
				                        "Expected_ " +
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
				                return std::unexpected(
				                    call_res.error().enrich_span(expr->span));
			                }
			                return call_res;
		                },
		                /* A */
		                [&, expr](std::shared_ptr<dara::backend::Class> cls)
		                    -> Result<Value> {
			                // 1029
			                if (arguments.size() != cls->arity() &&
			                    cls->arity() != static_cast<size_t>(-1)) {
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
				                // auto call_res_err = call_res.error();
				                // call_res_err.span = expr->span;
				                // return std::unexpected(call_res_err);
				                return std::unexpected(
				                    call_res.error().enrich_span(expr->span));
			                }
			                return call_res;
			                // return cls->call(*this, arguments);

			                /*
			                std::shared_ptr<dara::backend::Instance>
			                instance = cls->instantiate();

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
			                "Expected " + std::to_string(
			                constructor->arity())
			                + " argument but got " +
			                                std::to_string(arguments.size())
			                +
			                                "."});
			                        }

			                        auto call_res =
			                            constructor->call(*this,
			                arguments); if (!call_res) { return
			                std::unexpected( call_res.error());
			                        }
			                    }
			                } else {
			                    if (!arguments.empty()) {
			                        return
			                std::unexpected(dara::error::InterpreterError(
			                            "Expected 0 arguments but got "
			                + std::to_string(arguments.size())));
			                    }
			                }

			                return Value{instance};
			                */
		                },
		                [&](auto&&) -> Result<Value> {
			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "Can only call function and "
			                        "classes",
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
				                // auto inst_err = inst_res.error();
				                // inst_err.span = expr->span;
				                // return std::unexpected(inst_err);
				                // return
				                // std::unexpected(inst_res.error().enrich_span(expr->span));
				                return std::unexpected(
				                    inst_res.error().enrich_span(e.name.span));
			                }
			                return inst_res;
		                },
		                //  2.
		                //  クラスに対するスタティックメソッドアクセス
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
		                //[&](Array* array) -> Result<Value> {
		                /*
		                 [&](Array array) -> Result<Value> {
		                     if (e.name.lexeme == "size") {
		                         return Value{
		                             .data =
		                 static_cast<int>(array.size())};
		                     }
		                     return std::unexpected(
		                         dara::error::InterpreterError(
		                             "Array does not have proterty '" +
		                                 e.name.lexeme + "'.",
		                             e.name.span));
		                 },
		                 */
		                [&](std::shared_ptr<Array> array) -> Result<Value> {
			                return dara::backend::get_array_property(
			                    *this, array, e.name);

			                /*
			                if (e.name.lexeme == "size") {
			                    return Value{
			                        .data =
			                static_cast<int>(array->size())};
			                }
			                if (e.name.lexeme == "push") {
			                    std::shared_ptr<dara::backend::Callable>
			                        push_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayPush>(
			                            this->alloc, array);
			                    // return Value{.data =
			                std::allocate_shared<
			                    //           dara::backend::BuiltinNow>(
			                    //           this->alloc, array)};
			                    return Value{.data =
			                std::move(push_method)};
			                }
			                if (e.name.lexeme == "pop") {
			                    std::shared_ptr<dara::backend::Callable>
			                        pop_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayPop>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(pop_method)};
			                }
			                if (e.name.lexeme == "index") {
			                    std::shared_ptr<dara::backend::Callable>
			                        index_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayIndex>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(index_method)};
			                }
			                if (e.name.lexeme == "clear") {
			                    std::shared_ptr<dara::backend::Callable>
			                        clear_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayClear>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(clear_method)};
			                }

			                if (e.name.lexeme == "insert") {
			                    std::shared_ptr<dara::backend::Callable>
			                        insert_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinArrayInsert>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(insert_method)};
			                }

			                if (e.name.lexeme == "map") {
			                    std::shared_ptr<dara::backend::Callable>
			                        map_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayMap>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(map_method)};
			                }

			                if (e.name.lexeme == "filter") {
			                    std::shared_ptr<dara::backend::Callable>
			                        filter_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinArrayFilter>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(filter_method)};
			                }

			                if (e.name.lexeme == "reduce") {
			                    std::shared_ptr<dara::backend::Callable>
			                        filter_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinArrayReduce>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(filter_method)};
			                }

			                if (e.name.lexeme == "clone") {
			                    std::shared_ptr<dara::backend::Callable>
			                        clone_method = std::allocate_shared<
			                            dara::backend::BuiltinArrayClone>(
			                            this->alloc, array);
			                    return Value{.data =
			                std::move(clone_method)};
			                }

			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "Array does not have proterty '" +
			                            e.name.lexeme + "'.",
			                        e.name.span));
			                */
		                },
		                [&](std::shared_ptr<Map> map) -> Result<Value> {
			                return dara::backend::get_map_property(*this, map,
			                                                       e.name);

			                /*
			                if (e.name.lexeme == "clear") {
			                    std::shared_ptr<dara::backend::Callable>
			                        clear_method = std::allocate_shared<
			                            dara::backend::BuiltinMapClear>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(clear_method)};
			                }
			                if (e.name.lexeme == "has") {
			                    std::shared_ptr<dara::backend::Callable>
			                        has_method = std::allocate_shared<
			                            dara::backend::BuiltinMapHas>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(has_method)};
			                }
			                if (e.name.lexeme == "keys") {
			                    std::shared_ptr<dara::backend::Callable>
			                        keys_method = std::allocate_shared<
			                            dara::backend::BuiltinMapKeys>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(keys_method)};
			                }
			                if (e.name.lexeme == "values") {
			                    std::shared_ptr<dara::backend::Callable>
			                        values_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinMapValues>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(values_method)};
			                }
			                if (e.name.lexeme == "get") {
			                    std::shared_ptr<dara::backend::Callable>
			                        get_method = std::allocate_shared<
			                            dara::backend::BuiltinMapGet>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(get_method)};
			                }
			                if (e.name.lexeme == "clone") {
			                    std::shared_ptr<dara::backend::Callable>
			                        clone_method = std::allocate_shared<
			                            dara::backend::BuiltinMapClone>(
			                            this->alloc, map);
			                    return Value{.data =
			                std::move(clone_method)};
			                }

			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "Map does not have proterty '" +
			                            e.name.lexeme + "'.",
			                        e.name.span));
			                */
		                },
		                [&](String str) -> Result<Value> {
			                /*
			                if (e.name.lexeme == "contains") {
			                    std::shared_ptr<dara::backend::Callable>
			                        contains_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinStringContains>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(contains_method)};
			                }
			                */

			                /*
			                if (e.name.lexeme == "length") {
			                    std::shared_ptr<dara::backend::Callable>
			                        length_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinStringLength>(
			                            this->alloc, str);
			                    return Value{.data =
			                std::move(length_method)};
			                }
			                if (e.name.lexeme == "starts_with") {
			                    std::shared_ptr<dara::backend::Callable>
			                        startswith_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinStringStartsWith>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(startswith_method)};
			                }
			                if (e.name.lexeme == "ends_with") {
			                    std::shared_ptr<dara::backend::Callable>
			                        endswith_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinStringEndsWith>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(endswith_method)};
			                }
			                if (e.name.lexeme == "substring") {
			                    std::shared_ptr<dara::backend::Callable>
			                        substring_method =
			                std::allocate_shared<
			                            dara::backend::BuiltinStringSubstring>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(substring_method)};
			                }
			                if (e.name.lexeme == "split") {
			                    std::shared_ptr<dara::backend::Callable>
			                        split_method = std::allocate_shared<
			                            dara::backend::BuiltinStringSplit>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(split_method)};
			                }
			                if (e.name.lexeme == "trim") {
			                    std::shared_ptr<dara::backend::Callable>
			                        split_method = std::allocate_shared<
			                            dara::backend::BuiltinStringTrim>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(split_method)};
			                }
			                if (e.name.lexeme == "to_upper") {
			                    std::shared_ptr<dara::backend::Callable>
			                        upper_method = std::allocate_shared<
			                            dara::backend::BuiltinStringToUpper>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(upper_method)};
			                }
			                if (e.name.lexeme == "to_lower") {
			                    std::shared_ptr<dara::backend::Callable>
			                        lower_method = std::allocate_shared<
			                            dara::backend::BuiltinStringToLower>(
			                            this->alloc, str);
			                    return Value{.data =
			                                     std::move(lower_method)};
			                }

			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "String does not have proterty '" +
			                            e.name.lexeme + "'.",
			                        e.name.span));
			                */

			                return dara::backend::get_string_property(
			                    *this, str, e.name);
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
		        // PRINT_LINE();
		        //  return
		        //  std::unexpected(dara::error::InterpreterError{
		        //      "unimplemented expression evaluation",
		        //      expr->line, expr->col});
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
	                dara::error::InterpreterError{"unimplemented
	        expression evaluation", expr->line, expr->col});
	            // dara::error::InterpreterError {"unimplemented
	        expression
	            // evaluation"});::
	        },
	        */
	    },
	    expr->value);
}

// helper function
int Interpreter::factorial(int n) {
	if (n <= 1) return 1;
	auto rng = std::views::iota(1, n + 1);
	return std::accumulate(rng.begin(), rng.end(), 1LL, std::multiplies<int>());
}

}  // namespace dara::backend
