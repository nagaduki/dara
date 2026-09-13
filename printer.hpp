/* printer.hpp */

#pragma once

#include <string>
#include "ast.hpp"

class DotPrinter {
   public:
	std::string next_node_id();
	std::string print(const dara::ast::Expr* expr);
	std::string print(const std::vector<std::unique_ptr<dara::ast::Decl>>& program);
	std::expected<void, std::string> save_to_file(const dara::ast::Expr* expr);

	std::string print(const dara::ast::Decl* decl);
	std::string print(const dara::ast::Program* program);
	std::expected<void, std::string> save_to_file(const dara::ast::Decl* decl);

   private:
	std::string visit_expr(const dara::ast::Expr* expr);
	std::string visit_decl(const dara::ast::Decl* decl);
	std::string visit_stmt(const dara::ast::Stmt* stmt);
	int counter = 0;
	std::ostringstream out;
};

class StringPrinter {
   public:
	std::string print(const dara::ast::Expr* expr);
	std::string print(const dara::ast::Decl* decl);
	std::expected<void, std::string> save_to_file(const dara::ast::Expr* expr);

   private:
	std::string visit_expr(const dara::ast::Expr* expr);
	std::string visit_decl(const dara::ast::Decl* decl);
	std::string visit_stmt(const dara::ast::Stmt* stmt);

	std::ostringstream out;
};


