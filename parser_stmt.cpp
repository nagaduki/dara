/* parser_stmt.cpp */

#include <expected>
#include <iostream>
#include <utility>

#include "ast.hpp"
#include "combinator.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source.hpp"

// 現在のファイル名と行番号を出力するマクロ
#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace lox::frontend {

/* in parser_stmt.cpp */
/*
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::expr_stmt(
    std::unique_ptr<lox::Expr> expr) {
    if (!Semicolon(this->s)) {
        // PRINT_LINE();
        std::string error_message = std::format(
            "Expected ';' after expression {}", this->s->peek().value());
//(****)
        // return std::unexpected(s->make_error(std::format("Expected ';' after
        // expreression");
        return std::unexpected(s->make_error(error_message));
    }
    return std::make_unique<lox::Stmt>(lox::ExprStmt{std::move(expr)});
}
*/

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::expr_stmt(
    std::unique_ptr<lox::Expr> expr) {
	if (!Semicolon(this->s)) {
		auto peek_res = this->s->peek();
		std::string actual_char =
		    peek_res.has_value() ? std::string(1, peek_res.value()) : "EOF";

		std::string error_message = std::format(
		    "Expected ';' after expression, but got '{}'", actual_char);
		return std::unexpected(s->make_error(error_message));
	}
	return std::make_unique<lox::Stmt>(lox::ExprStmt{std::move(expr)});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::print_stmt() {
	int line = this->s->line;
	int col = this->s->col;

	auto expr_res = this->expr();

	if (!expr_res) return std::unexpected(expr_res.error());

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected ';' after value."));
	}

	return std::make_unique<lox::Stmt>(
	    lox::Stmt{.value = lox::PrintStmt{std::move(expr_res.value())},
	              .line = line,
	              .col = col});
}

/* in parser_stmt.cpp */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::assign_stmt(
    std::unique_ptr<lox::Expr> lhs) {
	auto rhs_res = this->expr();
	if (!rhs_res) return std::unexpected(rhs_res.error());

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after assign."));
	}
	return std::visit(
	    overloaded{
	        [&](lox::VarExpr& var_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(
		            lox::AssignStmt{.name = std::move(var_expr.name),
		                            .value = std::move(rhs_res.value())});
	        },
	        [&](lox::GetExpr& get_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(
		            lox::SetStmt{.object = std::move(get_expr.object),
		                         .name = std::move(get_expr.name),
		                         .value = std::move(rhs_res.value())});
	        },

	        [&](auto&)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target."));
	        }},
	    lhs->value);
}

/* in parser_stmt.cpp */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::add_assign_stmt(
    std::unique_ptr<lox::Expr> lhs) {
	auto rhs_res = this->expr();
	if (!rhs_res) return std::unexpected(rhs_res.error());

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after add assign."));
	}
	return std::visit(
	    overloaded{
	        [&](lox::VarExpr& var_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(lox::CompoundAssignStmt{
		            .name = std::move(var_expr.name),
		            .op = InfixOperator::Add,
		            .value = std::move(rhs_res.value())});
	        },
	        [&](lox::GetExpr& get_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(
		            lox::CompoundSetStmt{.object = std::move(get_expr.object),
		                                 .name = std::move(get_expr.name),
		                                 .op = InfixOperator::Add,
		                                 .value = std::move(rhs_res.value())});
	        },

	        [&](auto&)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target."));
	        }},
	    lhs->value);
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError>
Parser::compound_assign_stmt(std::unique_ptr<lox::Expr> lhs, InfixOperator op) {
	auto rhs_res = this->expr();
	if (!rhs_res) {
		return std::unexpected(rhs_res.error());
	}
	if (!Semicolon(this->s)) {
		return std::unexpected(
		    s->make_error("Expected ';' after compound assign"));
	}

	return std::visit(
	    overloaded{
	        [&](lox::VarExpr& var_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(lox::CompoundAssignStmt{
		            .name = std::move(var_expr.name),
		            .op = op,
		            .value = std::move(rhs_res.value())});
	        },
	        [&](lox::GetExpr& get_expr)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::make_unique<lox::Stmt>(
		            lox::CompoundSetStmt{.object = std::move(get_expr.object),
		                                 .name = std::move(get_expr.name),
		                                 .op = op,
		                                 .value = std::move(rhs_res.value())});
	        },
	        [&](auto&)
	            -> std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target."));
	        }},
	    lhs->value);
}
/*
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> _parse_brace_stmt(
    Source* s) {
    if (!LBrace(s)) return std::unexpected(s->make_error("not '{'"));

    auto inner_res = parse_stmt(s);
    if (!inner_res) return std::unexpected(inner_res.error());

    if (!RBrace(s)) return std::unexpected(s->make_error("expected '}'"));

    return std::move(inner_res.value());
}
*/

/* parse_brace_stmt */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::brace_stmt() {
	int line = s->line;
	int col = s->col;

	if (!LBrace(s)) {
		// PRINT_LINE();
		return std::unexpected(s->make_error("not '{'"));
	}

	std::vector<std::unique_ptr<lox::Decl>> declarations;

	while (!s->isEnd()) {
		if (RBrace(s)) {
			break;
		}

		auto decl_res = this->decl();

		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}

		declarations.push_back(std::move(decl_res.value()));
	}

	return std::make_unique<lox::Stmt>(
	    lox::Stmt{lox::BlockStmt{std::move(declarations)}, line, col});
}

/* parse_else_stmt */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::else_stmt() {
	int line = this->s->line;
	int col = this->s->col;
	auto then_res = this->brace_stmt();
	if (!then_res) return std::unexpected(then_res.error());

	return then_res;
}

/* parse_if_stmt */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::if_stmt() {
	int line = this->s->line;
	int col = this->s->col;

	auto condition_res = this->expr();
	if (!condition_res) return std::unexpected(condition_res.error());

	auto then_res = this->brace_stmt();
	if (!then_res) return std::unexpected(then_res.error());

	std::unique_ptr<lox::Stmt> else_res = nullptr;
	if (Else(this->s)) {
		auto else_block_res = this->brace_stmt();  //(*)
		if (!else_block_res) return std::unexpected(else_block_res.error());
		else_res = std::move(else_block_res.value());
	}

	return std::make_unique<lox::Stmt>(lox::Stmt{
	    .value = lox::IfStmt{std::move(condition_res.value()),
	                         std::move(then_res.value()), std::move(else_res)},
	    .line = line,
	    .col = col});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::while_stmt() {
	auto condition_res = this->expr();
	if (!condition_res) {
		return std::unexpected(condition_res.error());
	}

	LoopDepthGuard guard(this->loop_depth);

	auto body_res = this->brace_stmt();
	if (!body_res) {
		return std::unexpected(body_res.error());
	}

	return std::make_unique<lox::Stmt>(
	    lox::Stmt{.value = lox::WhileStmt{std::move(condition_res.value()),
	                                      std::move(body_res.value())}});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::forin_stmt() {
	if (!LParen(this->s))
		return std::unexpected(
		    this->s->make_error("Expected '(' after 'for'."));

	Let(this->s);  // 戻り値は無視して進める

	auto var_name_res = identifier(this->s);  // ご自身の識別子パース関数
	if (!var_name_res) {
		return std::unexpected(this->s->make_error("Expected variable name."));
	}
	std::string var_name = var_name_res.value();

	if (!In(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected 'in' after loop variable."));
	}
	auto iterable_res = this->expr(0);
	if (!iterable_res) return std::unexpected(iterable_res.error());

	if (!RParen(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected ')' after loop iterable."));
	}

	LoopDepthGuard guard(this->loop_depth);

	auto body_res = this->brace_stmt();
	if (!body_res) {
		return std::unexpected(body_res.error());
	}

	return std::make_unique<lox::Stmt>(lox::Stmt{
	    .value = lox::ForInStmt{.loop_variable = var_name,
	                            .iterable = std::move(iterable_res.value()),
	                            .body = std::move(body_res.value())}});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::inc_stmt(
    std::unique_ptr<lox::Expr> expr) {
	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after '++'."));
	}

	// 3. 左辺(expr)が変数(VarExpr)であるかチェック
	auto var_expr = std::get_if<lox::VarExpr>(&expr->value);
	if (!var_expr) {
		return std::unexpected(s->make_error("Invalid increment target."));
	}

	// 4. ASTノードを返す
	return std::make_unique<lox::Stmt>(
	    lox::Stmt{.value = lox::IncStmt{var_expr->name}});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::dec_stmt(
    std::unique_ptr<lox::Expr> expr) {
	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after '--'."));
	}

	// 3. 左辺(expr)が変数(VarExpr)であるかチェック
	auto var_expr = std::get_if<lox::VarExpr>(&expr->value);
	if (!var_expr) {
		return std::unexpected(s->make_error("Invalid decrement target."));
	}

	// 4. ASTノードを返す
	return std::make_unique<lox::Stmt>(
	    lox::Stmt{.value = lox::DecStmt{var_expr->name}});
}

/* in parser_stmt.cpp */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::return_stmt() {
	// PRINT_LINE();
	std::unique_ptr<lox::Expr> return_value = nullptr;

	if (Semicolon(this->s)) {
		PRINT_LINE();
		auto stmt = std::make_unique<lox::Stmt>();
		stmt->value = ReturnStmt{nullptr};
		return stmt;
	}

	auto expr_res = this->expr(0);
	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}

	return_value = std::move(expr_res.value());

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected ';' after return value"));
	}

	auto stmt = std::make_unique<lox::Stmt>();  //(*)
	stmt->value = ReturnStmt{std::move(return_value)};

	PRINT_LINE();
	return stmt;
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::break_stmt() {
	if (this->loop_depth == 0) {
		PRINT_LINE();
		return std::unexpected(
		    this->s->make_error("cannot use 'break' outside of a loop"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("expected ';' after 'break'"));
	}

	return std::make_unique<lox::Stmt>(lox::Stmt{.value = lox::BreakStmt{}});
}

std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::continue_stmt() {
	if (this->loop_depth == 0) {
		PRINT_LINE();
		return std::unexpected(
		    this->s->make_error("cannot use 'continue' outside of a loop"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("expected ';' after 'break'"));
	}

	return std::make_unique<lox::Stmt>(lox::Stmt{.value = lox::BreakStmt{}});
}

/* in parser_stmt.cpp */
/* parse_stmt */
std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> Parser::stmt() {
	this->s->skip_whitespace();  //(**)
	if (auto block_res = this->brace_stmt()) {
		return block_res;
	}

	if (Print(this->s)) {
		return this->print_stmt();
	}

	if (If(this->s)) {
		return this->if_stmt();
	}

	if (While(this->s)) {
		return this->while_stmt();
	}

	if (For(this->s)) {
		return this->forin_stmt();
	}

	if (Return(this->s)) {
		// PRINT_LINE();
		return this->return_stmt();
	}

	if (Break(this->s)) {
		return this->break_stmt();
	}

	if (Continue(this->s)) {
		return this->continue_stmt();
	}

	auto expr_res = this->expr();
	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}
	auto expr = std::move(expr_res.value());

	/* i++ */
	auto post_inc = Inc(this->s);
	if (post_inc) {
		return this->inc_stmt(std::move(expr));
	}

	/* i-- */
	auto post_dec = Dec(this->s);
	if (post_dec) {
		return this->dec_stmt(std::move(expr));
	}
	/*
*   // func(, class., array[ の処理 //
	auto post_lparen = LParen(this->s);
	if (post_lparen) {
	    return this->fn_stmt(std::move(expr));
	}
	auto post_dot = Dot(this->s);
	if (post_dot) {
	    return this->dot_stmt(std::move(expr));
	}
	auto post_lbracket =LBracket(this->s) (this->s);
	if (post_lbracket) {
	    return this->array_stmt(std::move(expr));
	}
	*/

	if (Assign(this->s)) {
		return this->assign_stmt(std::move(expr));
	}
	/* 0807 */
	if (AddAssign(this->s)) {
		return this->compound_assign_stmt(std::move(expr), InfixOperator::Add);
	}
	if (SubAssign(this->s)) {
		return this->compound_assign_stmt(std::move(expr), InfixOperator::Sub);
	}

	return this->expr_stmt(std::move(expr));  // old. TEST is OK
}

}  // namespace lox::frontend
