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
			        // initial_val = res.value();
			        initial_val = this->ensure_value_semantics(res.value());
		        }

		        this->env.get()->define(d.name.lexeme, initial_val);
		        // this.define(d.name, initial_val);
		        return {};
	        },

	        // __visit_decl__ClassDecl__
	        [this, decl](const ClassDecl& d) -> Result<void> {
		        //
		        this->env->define(d.name.lexeme,
		                          Value{std::monostate{}});  // (A)

		        // __visit_decl__super_class__
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
		        // __visit_decl__mixin__
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
		        // std::unordered_set<std::string, Value> fields;
		        // __visit_decl__generate_flat_members
		        auto is_field_val = [](const Value& value) -> bool {
			        return std::holds_alternative<std::monostate>(value.data);
		        };

		        // std::unordered_set<std::string>
		        std::unordered_map<std::string, Value> members;
		        std::vector<std::pair<std::string, const dara::ast::Expr*>>
		            field_initializers;

		        //
		        /*
		        if (super) {
		            fields.insert(super->fields.begin(), super->fields.end());
		            field_initializers.insert(field_initializers.end(),
		                                      super->field_initializers.begin(),
		                                      super->field_initializers.end());
		        }
		        */

		        if (super) {
			        members = super->members;
			        field_initializers = super->field_initializers;
		        }

		        std::unordered_set<std::string> mixin_methods;
		        for (const auto& mixin : mixins) {
			        if (!mixin) {
				        continue;
			        }
			        /*
			        for (const auto& mixin_field : mixin->fields) {
			            if (fields.contains(mixin_field)) {
			                return std::unexpected(
			                    dara::error::InterpreterError(
			                        "conflicting field '" + mixin_field +
			                            "' from mixin '" + mixin->name +
			                            "' in class '" + d.name.lexeme,
			                        decl->span));
			            }
			            fields.insert(mixin_field);
			        }
			        field_initializers.insert(field_initializers.end(),
			                                  mixin->field_initializers.begin(),
			                                  mixin->field_initializers.end());
			        */
			        for (const auto& [member_name, member_value] :
			             mixin->members) {
				        auto it = members.find(member_name);
				        if (it != members.end()) {
					        if (is_field_val(it->second) ||
					            is_field_val(member_value)) {
						        return std::unexpected(
						            dara::error::InterpreterError(
						                "conflicting field '" + member_name +
						                    "' from mixin '" + mixin->name +
						                    "' in class '" + d.name.lexeme +
						                    "'",
						                decl->span));
					        }
					        if (mixin_methods.contains(member_name)) {
						        return std::unexpected(
						            dara::error::InterpreterError(
						                "confilict method '" + member_name +
						                    "' between mixins in class '" +
						                    d.name.lexeme + "'",
						                decl->span));
					        }
				        }
				        if (!is_field_val(member_value)) {
					        mixin_methods.insert(member_name);
				        }
				        // must insert item to member
				        members[member_name] = member_value;
			        }
			        field_initializers.insert(field_initializers.end(),
			                                  mixin->field_initializers.begin(),
			                                  mixin->field_initializers.end());
		        }
		        /*
		        for (const auto& field_decl : d.fields) {
		            fields.insert(field_decl.name.lexeme);
		            if (field_decl.initializer != nullptr) {
		                field_initializers.emplace_back(
		                    field_decl.name.lexeme,
		                    field_decl.initializer.get());
		            }
		        }
		        */
		        // __visit_decl__regist_this_class_let_field_(value=monostate)
		        for (const auto& field_decl : d.fields) {
			        const std::string& this_field = field_decl.name.lexeme;
			        if (members.contains(this_field)) {
				        return std::unexpected(dara::error::InterpreterError(
				            "field '" + this_field + "' in class '" +
				                d.name.lexeme +
				                "' is already declared in superclass "
				                "or mixin "
				                "'" +
				                d.name.lexeme,
				            field_decl.span));
			        }
			        // members.insert(this_field); ng
			        // members.insert({this_field, Value{.data =
			        // std::monostate{}}}); //ok
			        members.emplace(
			            this_field,
			            Value{.data = std::monostate{}});  // ok with low cost

			        if (field_decl.initializer != nullptr) {
				        field_initializers.emplace_back(
				            this_field, field_decl.initializer.get());
			        }
		        }

		        OverloadedConstructor constructors;
		        // std::unordered_set<std::string, Value> this_methods;
		        std::unordered_set<std::string> this_methods;

		        /*
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
		                return
		        std::unexpected(dara::error::InterpreterError( "Method body
		        must be a FunctionExpr", decl->span));
		            }
		        }
		        */
		        // overloaded constructor generate.
		        // __visit_decl__constructor_and_method__
		        for (const auto& method_decl : d.methods) {
			        std::cout
			            << "[DEBUG] d.methods has: " << method_decl.name.lexeme
			            << std::endl;

			        if (auto* fn_expr = std::get_if<FunctionExpr>(
			                &method_decl.function->value)) {
				        auto function =
				            std::allocate_shared<dara::backend::Function>(
				                this->alloc, fn_expr, this->env);
				        const std::string& method_name =
				            method_decl.name.lexeme;
				        if (method_name == d.name.lexeme) {
					        if (!constructors.add(function)) {
						        return std::unexpected(
						            dara::error::InterpreterError(
						                "constructor '" + method_name +
						                    "' taking " +
						                    std::to_string(function->arity()) +
						                    " arguments is already defined "
						                    "in "
						                    "class '" +
						                    d.name.lexeme + "'",
						                //method_decl.span));
						                method_decl.name.span));
					        }
				        } else {
					        if (this_methods.contains(method_name)) {  //(***)
						        return std::unexpected(
						            dara::error::InterpreterError(
						                "duplicate method '" + method_name +
						                    "' in class '" + d.name.lexeme +
						                    "'",
						                //method_decl.span));
						                method_decl.name.span));
					        }
					        // __visit_decl__check_conflict
					        auto it = members.find(method_name);
					        if (it != members.end() &&
					            is_field_val(it->second)) {
						        return std::unexpected(
						            dara::error::InterpreterError(
						                "method '" + method_name +
						                    "' conflicts with a field in "
						                    "class "
						                    "'" +
						                    d.name.lexeme + "'",
						                //method_decl.span));
						                method_decl.name.span));
					        }
					        this_methods.insert(method_name);
					        members[method_name] = Value{function};
				        }
			        } else {
				        return std::unexpected(dara::error::InterpreterError(
				            "method body must be a FunctionExpr", decl->span));
			        }
		        }

		        // __decl_method__static_field

		        std::unordered_map<std::string, Value> static_members;
		        std::unordered_set<std::string> static_fields;
		        // static method
		        /*
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
		                return
		        std::unexpected(dara::error::InterpreterError( "Static
		        Method body must be a FunctionExpr", decl->span));
		            }
		        }
		        */
		        for (const auto& static_field : d.static_fields) {
			        std::cout << "[DEBUG] registering static field: "
			                  << static_field.name.lexeme << std::endl;
			        std::cout << "[DEBUG] registering static member: "
			                  << d.name.lexeme << std::endl;
			        std::cout << "[DEBUG] registering static method: "
			                  << static_field.name.lexeme << std::endl;

			        const std::string& static_field_name =
			            static_field.name.lexeme;
			        if (static_members.contains(static_field_name)) {
				        return std::unexpected(dara::error::InterpreterError(
				            "duplicate static member '" + static_field_name +
				                "' in classs '" + d.name.lexeme + "'",
				            static_field.span));
			        }
			        if (!static_field.initializer) {
				        return std::unexpected(dara::error::InterpreterError(
				            "static field '" + static_field_name +
				                "' in class '" + d.name.lexeme +
				                "' must have an initializer",
				            static_field.span));
			        }
			        auto init_res =
			            this->visit_expr(static_field.initializer.get());
			        if (!init_res) {
				        return std::unexpected(init_res.error());
			        }
			        static_fields.insert(static_field_name);
			        static_members[static_field_name] =
			            this->ensure_value_semantics(init_res.value());
		        }
		        // __decl_method__static_method
		        // std::unordered_map<std::string, Value>
		        // static_methods;
		        for (const auto& static_method_decl : d.static_methods) {
			        if (auto* static_fn_expr = std::get_if<FunctionExpr>(
			                &static_method_decl.function->value)) {
				        const std::string static_method_name =
				            static_method_decl.name.lexeme;
				        // if (static_methods.contains(
				        if (static_members.contains(static_method_name)) {
					        return std::unexpected(
					            dara::error::InterpreterError(
					                "duplicate static method '" +
					                    static_method_name + "' in class '" +
					                    d.name.lexeme + "'",
					                static_method_decl.span));
				        }
				        auto function =
				            std::allocate_shared<dara::backend::Function>(
				                this->alloc, static_fn_expr, this->env);
				        static_members[static_method_name] =
				            Value{.data = function};
			        } else {
				        return std::unexpected(dara::error::InterpreterError(
				            "static method body must be a "
				            "FunctionExpr",
				            decl->span));
			        }
		        }

		        // std::unordered_map<std::string, Value> static_members;
		        // std::unordered_map<std::string> static_fields;

		        // auto cls = std::make_shared<dara::backend::Class>(
		        // __decl_method__static_method
		        auto cls = std::allocate_shared<dara::backend::Class>(
		            this->alloc, d.name.lexeme, std::move(super),
		            std::move(mixins), std::move(members),
		            std::move(static_members), std::move(static_fields),
		            std::move(constructors), std::move(field_initializers));

		        // cls->super = std::move(super);
		        // cls->mixins = std::move(mixins);

		        // [DEBUG]

		        this->env->assign(d.name.lexeme, Value{cls});  //(B)
		        return {};
	        },

	        /*
	        // ClassDecl
	        [this, decl](const ClassDecl& d) -> Result<void> {
	            //
	            this->env->define(d.name.lexeme,
	                              Value{std::monostate{}});  // (A)

	            // super class
	            std::shared_ptr<dara::backend::Class> super =
	        nullptr; if (d.super) { auto super_val =
	        this->visit_expr(d.super.get()); if (!super_val) {
	                    return std::unexpected(super_val.error());
	                }
	                if (auto* class_ptr = std::get_if<
	                        std::shared_ptr<dara::backend::Class>>(
	        // (C) &super_val->data)) { super = *class_ptr; } else {
	                    // throw
	        dara::error::InterpreterError("super class must
	                    // bea class");
	                    return
	        std::unexpected(dara::error::InterpreterError(
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
	            std::vector<std::shared_ptr<dara::backend::Class>>
	        mixins; for (const auto& mixin_expr : d.mixins) { auto
	        mixin_val = this->visit_expr(mixin_expr.get()); if
	        (!mixin_val) { return
	        std::unexpected(mixin_val.error());
	                }
	                if (auto* class_ptr =
	                        std::get_if<std::shared_ptr<dara::backend::Class>>(
	                            &mixin_val->data)) {
	                    mixins.push_back(*class_ptr);
	                } else {
	                    return
	        std::unexpected(dara::error::InterpreterError( "Mixin
	        must be a class", decl->span));
	                }
	            }

	            // method
	            std::unordered_map<std::string, Value> methods;
	            for (const auto& method_decl : d.methods) {
	                if (auto* fn_expr = std::get_if<FunctionExpr>(
	                        &method_decl.function->value)) {
	                    auto function =
	                        //
	        std::make_shared<dara::backend::Function>(
	                        std::allocate_shared<dara::backend::Function>(
	                            // fn_expr, this->env);
	                            this->alloc, fn_expr, this->env);

	                    methods[method_decl.name.lexeme] =
	        Value{function}; } else {
	                    // throw dara::error::InterpreterError(
	        "method body
	                    // must be FunctionExpr");
	                    return
	        std::unexpected(dara::error::InterpreterError( "Method
	        body must be a FunctionExpr", decl->span));
	                }
	            }

	            // static method
	            std::unordered_map<std::string, Value>
	        static_methods; for (const auto& static_method_decl :
	        d.static_methods) { if (auto* static_fn_expr =
	        std::get_if<FunctionExpr>(
	                        &static_method_decl.function->value)) {
	                    auto function =
	                        //
	        std::make_shared<dara::backend::Function>(
	                        std::allocate_shared<dara::backend::Function>(
	                            // static_fn_expr, this->env);
	                            this->alloc, static_fn_expr,
	        this->env);

	                    static_methods[static_method_decl.name.lexeme]
	        = Value{.data = function}; } else {
	                    //
	                    return
	        std::unexpected(dara::error::InterpreterError( "Static
	        Method body must be a FunctionExpr", decl->span));
	                }
	            }
	            // auto cls =
	        std::make_shared<dara::backend::Class>( auto cls =
	        std::allocate_shared<dara::backend::Class>( this->alloc,
	        d.name.lexeme, std::move(super), std::move(mixins),
	        std::move(methods), std::move(static_methods));

	            // cls->super = std::move(super);
	            // cls->mixins = std::move(mixins);

	            this->env->assign(d.name.lexeme, Value{cls});  //(B)
	            return {};
	        },
	        */

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
}


}
