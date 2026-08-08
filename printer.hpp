/* printer.hpp */

#pragma once

#include <string>
#include "ast.hpp"

class DotPrinter {
   public:
	std::string next_node_id();
	std::string print(const lox::Expr* expr);
	std::string print(const std::vector<std::unique_ptr<lox::Decl>>& program);
	std::expected<void, std::string> save_to_file(const lox::Expr* expr);

	std::string print(const lox::Decl* decl);
	std::string print(const lox::Program* program);
	std::expected<void, std::string> save_to_file(const lox::Decl* decl);

   private:
	std::string visit_expr(const lox::Expr* expr);
	std::string visit_decl(const lox::Decl* decl);
	std::string visit_stmt(const lox::Stmt* stmt);
	int counter = 0;
	std::ostringstream out;
};

class StringPrinter {
   public:
	std::string print(const lox::Expr* expr);
	std::string print(const lox::Decl* decl);
	std::expected<void, std::string> save_to_file(const lox::Expr* expr);

   private:
	std::string visit_expr(const lox::Expr* expr);
	std::string visit_decl(const lox::Decl* decl);
	std::string visit_stmt(const lox::Stmt* stmt);

	std::ostringstream out;
};


