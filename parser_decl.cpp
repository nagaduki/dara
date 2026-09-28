/* parser_decl.cpp */

#include <expected>
#include <iostream>
#include <utility>
#include <variant>

#include "ast.hpp"
#include "combinator.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source.hpp"

using namespace dara::combinator;
using namespace dara::lexer;
using namespace dara::ast;

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

namespace dara::frontend {

std::expected<std::unique_ptr<Decl>, dara::error::SyntaxError>
Parser::var_decl() {
	this->s->skip_whitespace();

	// auto space_res = this->s->skip_whitespace();
	// if (!space_res) {
	//	return std::unexpected(space_res.error());
	// }

	// int line = this->s->line;
	// int col = this->s->col;

	size_t start = s->get_current();
	// variable name
	auto var_res = identifier(s);

	if (!var_res) return std::unexpected(var_res.error());

	if (!sym('=')(this->s))
		return std::unexpected(
		    this->s->make_error("expected '=' after variable"));

	auto expr_res = this->expr();
	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}

	/*
	if (this->function_depth > 0 &&
	    // std::holds_alternative<dara::FunctionExpr>(expr_res.value()->value))
	{ std::holds_alternative<FunctionExpr>((*expr_res)->value)) { return
	std::unexpected(this->s->make_error( "cannnot assign a function to a
	variable insdie a local scope "));
	}
	*/

	if (!Semicolon(this->s)) {
		PRINT_LINE();
		return std::unexpected(this->s->make_error(
		    "expected ';' at the end of let statement."));  //(*)
	}

	size_t end = s->get_current();
	return std::make_unique<Decl>(
	    Decl{.value = VarDecl{.name = var_res.value(),
	                          .initializer = std::move(expr_res.value())},
	         .span = Span{start, end}});
}

std::expected<std::unique_ptr<Decl>, dara::error::SyntaxError>
Parser::fn_decl() {
	this->s->skip_whitespace();
	// auto space_res = this->s->skip_whitespace();
	// if (!space_res) {
	//	return std::unexpected(space_res.error());
	// }

	// global check //
	/*
	if (this->function_depth > 0) {
	    return std::unexpected(this->s->make_error(
	        "Named function declaration are only allowed at the top
	level"));
	}
	*/

	size_t start = s->get_current();
	size_t end = start + 1;

	auto name_res = identifier(this->s);
	end = s->get_current();

	if (!name_res) {
		// return std::unexpected(this->s->make_error("Expected function name",
		// Span{start,end}));
		return std::unexpected(this->s->make_error("Expected function name"));
	}
	// std::string name = name_res.value();
	dara::ast::Identifier name = name_res.value();

	if (!LParen(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '(' after funtion name"));
	}

	// std::vector<std::string> parameters;
	std::vector<dara::ast::Identifier> parameters;

	if (!RParen(this->s)) {
		do {
			auto param_res = identifier(this->s);
			if (!param_res) {
				return std::unexpected(param_res.error());
			}
			parameters.push_back(param_res.value());
		} while (Comma(this->s));

		if (!RParen(this->s)) {
			return std::unexpected(
			    this->s->make_error("Expected ')' after parameter"));
		}
	}

	if (!LBrace(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '{' after parameter"));
	}

	std::vector<std::unique_ptr<Decl>> body;

	// FunctionDepthGuard guard(this->function_depth, this->loop_depth);

	while (!RBrace(this->s) && !this->s->isEnd()) {
		auto decl_res = this->decl();

		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}
		body.push_back(std::move(decl_res.value()));
	}

	end = s->get_current();

	auto function_expr = std::make_unique<Expr>();
	function_expr->value = FunctionExpr{std::move(parameters), std::move(body)};
	function_expr->span = dara::core::Span{start, end};

	return std::make_unique<Decl>(
	    Decl{.value = VarDecl{std::move(name), std::move(function_expr)},
	         .span = Span{start, end}});
}

// class_decl in parser_decl
std::expected<std::unique_ptr<Decl>, dara::error::SyntaxError>
Parser::class_decl() {
	this->s->skip_whitespace();
	// Source backup = *(this->s);

	size_t start = s->get_current();
	size_t end = start + 1;

	auto name_res = identifier(this->s);
	if (!name_res) {
		return std::unexpected(this->s->make_error("Expected class name"));
	}

	// std::string class_name = name_res.value();
	dara::ast::Identifier class_name = name_res.value();
	std::unique_ptr<Expr> super = nullptr;

	if (Extends(this->s)) {
		size_t super_start = s->get_current();
		auto super_name_res = identifier(this->s);
		if (!super_name_res) {
			return std::unexpected(this->s->make_error(
			    //"Expected super class name after 'extends'", Span{start,
			    // end}));
			    "Expected super class name after 'extends'"));
		}
		size_t super_end = s->get_current();
		auto var_expr = std::make_unique<Expr>();
		var_expr->value = VarExpr{.name = super_name_res.value()};
		var_expr->span = Span{super_start, super_end};
		super = std::move(var_expr);
	}

	std::vector<std::unique_ptr<Expr>> mixins;

	if (With(this->s)) {
		do {
			size_t mix_start = s->get_current();
			auto mixin_res = identifier(this->s);
			end = s->get_current();
			if (!mixin_res) {
				return std::unexpected(this->s->make_error(
				    "Expected mixin name after 'with'", Span{start, end}));
			}
			size_t mix_end = s->get_current();
			auto var_expr = std::make_unique<Expr>();
			var_expr->value = VarExpr{.name = mixin_res.value()};
			var_expr->span = Span{mix_start, mix_end};
			mixins.push_back(std::move(var_expr));

		} while (Comma(this->s));
	}

	if (!LBrace(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '{' before class body"));
	}

	std::vector<MethodDecl> methods;
	std::vector<MethodDecl> static_methods;

	// this->s->skip_whitespace();
	while (!RBrace(this->s) && !this->s->isEnd()) {
		size_t method_start = s->get_current();
		//std::string method_name;
		Identifier method_name;
		bool is_static = false;

		if (Static(this->s)) {
			is_static = true;
			this->s->skip_whitespace();
			if (!Function(this->s)) {
				return std::unexpected(this->s->make_error(
				    "Expected 'fn' before method declaration"));
			}

			/* method_name_res */
			auto method_name_res = identifier(this->s);
			if (!method_name_res) {
				return std::unexpected(this->s->make_error(
				    "Expected method name after 'static fn'"));
			}

			method_name = method_name_res.value();

		} else if (Function(this->s)) {
			auto method_name_res = identifier(this->s);
			if (!method_name_res) {
				return std::unexpected(
				    this->s->make_error("Expected mathod name after 'fn'"));
			}

			// std::string method_name = method_name_res.value();
			method_name = method_name_res.value();
		} else {
			auto method_name_res = identifier(this->s);
			if (!method_name_res) {
				/* 0921 next */
				return std::unexpected(this->s->make_error(
				    "Expected 'fn', 'static', or constructor declaration"));
			}
			method_name = method_name_res.value();

			//if (method_name != class_name) {
			if (method_name.lexeme != class_name.lexeme) {
				return std::unexpected(this->s->make_error(
				    "Missing 'fn' for method, or constructor name '" +
				    method_name.lexeme + "' does not match class name '" + class_name.lexeme +
				    "'"));
			}
		}

		if (!LParen(this->s)) {
			return std::unexpected(
			    this->s->make_error("Expected '(' after method name"));
		}

		//std::vector<std::string> parameters;
		std::vector<dara::ast::Identifier> parameters;
		if (!RParen(this->s)) {
			do {
				auto param_res = identifier(this->s);
				if (!param_res) {
					return std::unexpected(param_res.error());
				}
				parameters.push_back(param_res.value());
			} while (Comma(this->s));
			if (!RParen(this->s)) {
				return std::unexpected(
				    this->s->make_error("Expected ')' after	parameter"));
			}
		}

		FunctionDepthGuard guard(this->function_depth, this->loop_depth);

		//  1. 実績のある brace_stmt() に '{ ... }' のパースを丸投げする
		auto block_res = this->brace_stmt();
		if (!block_res) {
			return std::unexpected(block_res.error());
		}

		//  2. 返ってきた Stmt から BlockStmt の中身 (vector<Decl>)
		//  を抽出する
		auto* stmt_ptr = block_res.value().get();
		// std::variant から BlockStmt を取り出す
		auto& block_stmt = std::get<BlockStmt>(stmt_ptr->value);
		// declarations をメソッドの body としてムーブする
		std::vector<std::unique_ptr<Decl>> body =
		    std::move(block_stmt.declarations);

		size_t method_end = s->get_current();

		auto fn_expr_node = std::make_unique<Expr>();
		fn_expr_node->value =
		    FunctionExpr{std::move(parameters), std::move(body)};
		fn_expr_node->span = Span{method_start, method_end};
		if (is_static) {
			static_methods.push_back(
			    MethodDecl{.name = std::move(method_name),
			               .function = std::move(fn_expr_node)});
		} else {
			methods.push_back(MethodDecl{.name = std::move(method_name),
			                             .function = std::move(fn_expr_node)});
		}
		// this->s->skip_whitespace();
	}

	auto class_decl_node =
	    ClassDecl{.name = std::move(class_name),
	              .super = std::move(super),
	              .mixins = std::move(mixins),
	              .methods = std::move(methods),
	              .static_methods = std::move(static_methods)};

	end = s->get_current();
	// Decl にラップして、std::expected (Result) として返す
	return std::make_unique<Decl>(
	    Decl{.value = std::move(class_decl_node), .span = Span{start, end}});
}

std::expected<std::unique_ptr<Decl>, dara::error::SyntaxError> Parser::decl() {
	this->s->skip_whitespace();  //(**)
	// auto space_res = this->s->skip_whitespace();
	// if (!space_res) {
	//	return std::unexpected(space_res.error());
	// }

	TraceGuard trace("parse_decl");
	Source backup = *s;

	size_t start = s->get_current();

	if (Class(this->s)) {
		return this->class_decl();
	}

	*(this->s) = backup;

	if (Function(this->s)) {
		return this->fn_decl();
	}

	*(this->s) = backup;

	if (Let(this->s)) {
		return this->var_decl();
	}

	*(this->s) = backup;

	auto stmt_res = this->stmt();
	if (!stmt_res) {
		return std::unexpected(stmt_res.error());
	}

	size_t end = s->get_current();
	// int line = (*stmt_res)->line;
	// int col = (*stmt_res)->col;

	return std::make_unique<Decl>(
	    Decl{.value = TopLevelStmt{std::move(stmt_res.value())},
	         .span = Span{start, end}});
	//.line = line,
	//.col = col});
}

std::expected<Program, dara::error::SyntaxError> Parser::program() {
	std::vector<std::unique_ptr<Decl>> declarations;

	while (true) {
		this->s->skip_whitespace();
		// auto space_res = this->s->skip_whitespace();
		// if (!space_res) {
		//	return std::unexpected(space_res.error());
		// }

		// spaces(this->s);

		if (this->s->isEnd()) break;

		auto decl_res = this->decl();
		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}
		declarations.push_back(std::move(decl_res.value()));
	}

	return Program{std::move(declarations)};
}

}  // namespace dara::frontend
//}  // namespace dara::frontend
/*
std::expected<dara::Program, dara::error::SyntaxError> __parse_program(Source*
s) { std::vector<std::unique_ptr<dara::Decl>> declarations;

    while (true) {
        while (!s->isEnd()) {
            // if (auto c_res = spaces(s)) {
            auto c_res = s->peek();
            if (!c_res || isSpace(c_res.value())) {
                s->next();
            } else {
                break;
            }
        }
        auto decl_res = parse_decl(s);
        if (!decl_res) {
            return std::unexpected(decl_res.error());
        }
        declarations.push_back(std::move(decl_res.value()));
    }

    return dara::Program{std::move(declarations)};
}

std::expected<dara::Program, dara::error::SyntaxError> _parse_program(Source*
s) { std::vector<std::unique_ptr<dara::Decl>> declarations;

    while (!s->isEnd()) {
        auto decl_res = parse_decl(s);
        if (!decl_res) {
            return std::unexpected(decl_res.error());
        }
        declarations.push_back(std::move(decl_res.value()));
    }

    return dara::Program{std::move(declarations)};
}
*/
/* parse_program */
/*
std::expected<std::unique_ptr<dara::Decl>, dara::error::SyntaxError>
_parse_program(Source* s) { std::vector<std::unique_ptr<dara::Decl>>
declarations;

    while (!s->isEnd()) {
        auto decl_res = parse_decl(s);

        if (!decl_res) {
            return std::unexpected(decl_res.error());
        }

        declarations.push_back(std::move(decl_res.value()));
    }

    return std::make_unique<dara::Decl>(
        dara::Decl{
dara::Program{std::move(declarations)}, 1, 1});
}
*/
