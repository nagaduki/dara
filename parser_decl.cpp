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

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

namespace lox::frontend {

std::expected<std::unique_ptr<lox::Decl>, SyntaxError> Parser::var_decl() {
	int line = this->s->line;
	int col = this->s->col;
	auto var_res = identifier(s);

	if (!var_res) return std::unexpected(var_res.error());

	if (!sym('=')(this->s))
		return std::unexpected(
		    this->s->make_error("expected '=' after variable"));

	auto expr_res = this->expr();
	if (!expr_res) return std::unexpected(expr_res.error());

	if (this->function_depth > 0 &&
	    // std::holds_alternative<lox::FunctionExpr>(expr_res.value()->value)) {
	    std::holds_alternative<lox::FunctionExpr>((*expr_res)->value)) {
		return std::unexpected(this->s->make_error(
		    "cannnot assign a function to a variable insdie a local scope "));
	}

	if (!Semicolon(this->s)) {
		PRINT_LINE();
		return std::unexpected(this->s->make_error(
		    "expected ';' at the end of let statement."));  //(*)
	}

	return std::make_unique<lox::Decl>(lox::Decl{
	    .value = lox::VarDecl{var_res.value(), std::move(expr_res.value())},
	    .line = line,
	    .col = col});
}

std::expected<std::unique_ptr<lox::Decl>, SyntaxError> Parser::fn_decl() {
	// global check //
	if (this->function_depth > 0) {
		return std::unexpected(this->s->make_error(
		    "Named function declaration are only allowed at the top level"));
	}

	auto name_res = identifier(this->s);
	if (!name_res) {
		return std::unexpected(this->s->make_error("Expected function name"));
	}
	std::string name = name_res.value();

	if (!LParen(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '(' after funtion name"));
	}

	std::vector<std::string> parameters;

	// (**)
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
	// (**)

	if (!LBrace(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '{' after parameter"));
	}

	std::vector<std::unique_ptr<lox::Decl>> body;

	FunctionDepthGuard guard(this->function_depth, this->loop_depth);

	while (!RBrace(this->s) && !this->s->isEnd()) {
		auto decl_res = this->decl();

		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}
		body.push_back(std::move(decl_res.value()));
	}

	auto function_expr = std::make_unique<lox::Expr>();
	function_expr->value = FunctionExpr{std::move(parameters), std::move(body)};

	return std::make_unique<lox::Decl>(lox::Decl{
	    .value = lox::VarDecl{std::move(name), std::move(function_expr)}});
}

// class_decl in parser_decl
std::expected<std::unique_ptr<lox::Decl>, SyntaxError> Parser::class_decl() {
	auto name_res = identifier(this->s);
	if (!name_res) {
		return std::unexpected(this->s->make_error("Expected class name"));
	}

	std::string class_name = name_res.value();
	std::unique_ptr<lox::Expr> super = nullptr;

	if (Extends(this->s)) {
		auto super_name_res = identifier(this->s);
		if (!super_name_res) {
			return std::unexpected(this->s->make_error(
			    "Expected super class name after 'extends'"));
		}
		auto var_expr = std::make_unique<lox::Expr>();
		var_expr->value = lox::VarExpr{.name = super_name_res.value()};
		super = std::move(var_expr);
	}

	std::vector<std::unique_ptr<lox::Expr>> mixins;

	if (With(this->s)) {
		do {
			auto mixin_res = identifier(this->s);
			if (!mixin_res) {
				return std::unexpected(
				    this->s->make_error("Expected minin name after 'with'"));
			}
			auto var_expr = std::make_unique<lox::Expr>();
			var_expr->value = lox::VarExpr{.name = mixin_res.value()};
			mixins.push_back(std::move(var_expr));

		} while (Comma(this->s));
	}

	if (!LBrace(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '{' before class body"));
	}

	std::vector<MethodDecl> methods;

	while (!RBrace(this->s) && !this->s->isEnd()) {
		if (!Function(this->s)) {
			return std::unexpected(
			    this->s->make_error("Expected 'fn' before method declaration"));
		}

		auto method_name_res = identifier(this->s);
		if (!method_name_res) {
			return std::unexpected(
			    this->s->make_error("Expected mathod name after 'fn'"));
		}

		std::string method_name = method_name_res.value();
		if (!LParen(this->s)) {
			return std::unexpected(
			    this->s->make_error("Expected '(' after method name"));
		}
		std::vector<std::string> parameters;
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

		// 🌟 1. 実績のある brace_stmt() に '{ ... }' のパースを丸投げする
		auto block_res = this->brace_stmt();
		if (!block_res) {
			return std::unexpected(block_res.error());
		}

		// 🌟 2. 返ってきた Stmt から BlockStmt の中身 (vector<Decl>) を抽出する
		auto* stmt_ptr = block_res.value().get();
		// std::variant から BlockStmt を取り出す
		auto& block_stmt = std::get<lox::BlockStmt>(stmt_ptr->value);
		// declarations をメソッドの body としてムーブする
		std::vector<std::unique_ptr<lox::Decl>> body =
		    std::move(block_stmt.declarations);

		auto fn_expr_node = std::make_unique<lox::Expr>();
		fn_expr_node->value =
		    FunctionExpr{std::move(parameters), std::move(body)};

		methods.push_back(MethodDecl{.name = std::move(method_name),
		                             .function = std::move(fn_expr_node)});
	}
	auto class_decl_node = lox::ClassDecl{.name = std::move(class_name),
	                                      .super = std::move(super),
	                                      .mixins = std::move(mixins),
	                                      .methods = std::move(methods)};

	// Decl にラップして、std::expected (Result) として返す
	return std::make_unique<lox::Decl>(
	    lox::Decl{.value = std::move(class_decl_node)});
}

std::expected<std::unique_ptr<lox::Decl>, SyntaxError> Parser::decl() {
	this->s->skip_whitespace();  //(**)
	TraceGuard trace("parse_decl");
	Source backup = *s;

	if (Class(this->s)) {
		return this->class_decl();
	}

	if (Function(this->s)) {
		return this->fn_decl();
	}

	if (Let(this->s)) {
		return this->var_decl();
	}

	*(this->s) = backup;

	auto stmt_res = this->stmt();
	if (!stmt_res) return std::unexpected(stmt_res.error());

	int line = (*stmt_res)->line;
	int col = (*stmt_res)->col;

	return std::make_unique<lox::Decl>(
	    lox::Decl{.value = lox::TopLevelStmt{std::move(stmt_res.value())},
	              .line = line,
	              .col = col});
}

std::expected<lox::Program, SyntaxError> Parser::program() {
	std::vector<std::unique_ptr<lox::Decl>> declarations;

	while (true) {
		this->s->skip_whitespace();
		spaces(this->s);
		if (this->s->isEnd()) break;

		auto decl_res = this->decl();
		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}
		declarations.push_back(std::move(decl_res.value()));
	}

	return lox::Program{std::move(declarations)};
}

}  // namespace lox::frontend
//}  // namespace lox::frontend
/*
std::expected<lox::Program, SyntaxError> __parse_program(Source* s) {
    std::vector<std::unique_ptr<lox::Decl>> declarations;

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

    return lox::Program{std::move(declarations)};
}

std::expected<lox::Program, SyntaxError> _parse_program(Source* s) {
    std::vector<std::unique_ptr<lox::Decl>> declarations;

    while (!s->isEnd()) {
        auto decl_res = parse_decl(s);
        if (!decl_res) {
            return std::unexpected(decl_res.error());
        }
        declarations.push_back(std::move(decl_res.value()));
    }

    return lox::Program{std::move(declarations)};
}
*/
/* parse_program */
/*
std::expected<std::unique_ptr<lox::Decl>, SyntaxError>
_parse_program(Source* s) { std::vector<std::unique_ptr<lox::Decl>>
declarations;

    while (!s->isEnd()) {
        auto decl_res = parse_decl(s);

        if (!decl_res) {
            return std::unexpected(decl_res.error());
        }

        declarations.push_back(std::move(decl_res.value()));
    }

    return std::make_unique<lox::Decl>(
        lox::Decl{
lox::Program{std::move(declarations)}, 1, 1});
}
*/
