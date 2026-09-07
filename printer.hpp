/* printer.hpp */

#pragma once

#include <string>
#include "ast.hpp"

class DotPrinter {
   public:
	std::string next_node_id();
	std::string print(const dara::Expr* expr);
	std::string print(const std::vector<std::unique_ptr<dara::Decl>>& program);
	std::expected<void, std::string> save_to_file(const dara::Expr* expr);

	std::string print(const dara::Decl* decl);
	std::string print(const dara::Program* program);
	std::expected<void, std::string> save_to_file(const dara::Decl* decl);

   private:
	std::string visit_expr(const dara::Expr* expr);
	std::string visit_decl(const dara::Decl* decl);
	std::string visit_stmt(const dara::Stmt* stmt);
	int counter = 0;
	std::ostringstream out;
};

class StringPrinter {
   public:
	std::string print(const dara::Expr* expr);
	std::string print(const dara::Decl* decl);
	std::expected<void, std::string> save_to_file(const dara::Expr* expr);

   private:
	std::string visit_expr(const dara::Expr* expr);
	std::string visit_decl(const dara::Decl* decl);
	std::string visit_stmt(const dara::Stmt* stmt);

	std::ostringstream out;
};


