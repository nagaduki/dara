/* printer */

#include "printer.hpp"

#include <string>

#include "ast.hpp"

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string StringPrinter::print(const lox::Expr* expr) {
	// this->visit_expr(expr);

	return this->visit_expr(expr);
}

std::string StringPrinter::print(const lox::Decl* decl) {
	// this->visit_expr(expr);
	return this->visit_decl(decl);
}

std::string StringPrinter::visit_expr(const lox::Expr* expr) {
	return std::visit(
	    overloaded{[](const lox::IntExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const lox::StringExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const lox::CharExpr& expr) {
		               return std::format("'{}'", expr.value);
	               },
	               [](const lox::VarExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.name);  //(*)
	               },
	               [](const lox::BoolExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.value);  //(*)
	               },
	               [this](const lox::CallExpr& expr) -> std::string {
		               // dummy
		               return "CallExpr";
	               },
	               [this](const lox::ArrayExpr& expr) -> std::string {
		               // dummy
		               return "ArrayExpr";
	               },
	               [this](const lox::IndexExpr& expr) -> std::string {
		               // dummy
		               return "IndexExpr";
	               },
	               [this](const lox::GetExpr& expr) -> std::string {
		               // dummy
		               return "GetExpr";
	               },
	               [this](const lox::ThisExpr& expr) -> std::string {
		               // dummy
		               return "ThisExpr";
	               },

	               [this](const lox::FunctionExpr& e) -> std::string {
		               return "FunctionExpr";
	               },
	               [this](const lox::InfixOpExpr& expr) {
		               std::string op_str;
		               switch (expr.op) {
			               case InfixOperator::Add:
				               op_str = "+";
				               break;
			               case InfixOperator::Sub:
				               op_str = "-";  //*
				               break;         //**
			               case InfixOperator::Mul:
				               op_str = "*";
				               break;
			               case InfixOperator::Div:
				               op_str = "/";
				               break;
			               case InfixOperator::Pow:
				               op_str = "^";
				               break;
			               case InfixOperator::Assign:
				               op_str = "=";
				               break;
			               case InfixOperator::Greater:
				               op_str = ">";
				               break;
			               case InfixOperator::GreaterEqual:
				               op_str = ">=";
				               break;
			               case InfixOperator::Less:
				               op_str = "<";
				               break;
			               case InfixOperator::LessEqual:
				               op_str = "<=";
				               break;
			               case InfixOperator::EqualEqual:
				               op_str = "==";
				               break;
			               case InfixOperator::NotEqual:
				               op_str = "!=";
				               break;
		               }
		               return std::format("({} {} {})", op_str,
		                                  visit_expr(expr.lhs.get()),
		                                  visit_expr(expr.rhs.get()));
	               },
	               [this](const lox::LogicalOpExpr& expr) {
		               std::string op_str;
		               switch (expr.op) {
			               case LogicalOperator::And:
				               op_str = "and";
				               break;
			               case LogicalOperator::Or:
				               op_str = "or";
				               break;
		               }
		               return std::format("({} {} {})", op_str,
		                                  visit_expr(expr.lhs.get()),
		                                  visit_expr(expr.rhs.get()));
	               },
	               [this](const lox::PrefixOpExpr& expr) {
		               std::string op_str;
		               switch (expr.op) {
			               case PrefixOperator::Pos:
				               op_str = "+";
				               break;
			               case PrefixOperator::Neg:
				               op_str = "-";
				               break;
			               case PrefixOperator::Not:
				               op_str = "!";
				               break;
				               /*
				              case PrefixOperator::Inc:
				                  op_str = "++";
				                  break;
				              case PrefixOperator::Dec:
				                  op_str = "--";
				                  break;
				               */
		               }
		               return std::format("({} {})", op_str,
		                                  visit_expr(expr.rhs.get()));
	               },
	               [this](const lox::PostfixOpExpr& expr) {
		               std::string op_str;
		               switch (expr.op) {
				               /*
				              case PostfixOperator::Inc:
				                  op_str = "++";
				                  break;
				              case PostfixOperator::Dec:
				                  op_str = "--";
				                  break;
				               */
			               case PostfixOperator::Fac:
				               op_str = "!";
				               break;
		               }
		               return std::format("({} {})", op_str,
		                                  visit_expr(expr.lhs.get()));
	               }},
	    expr->value);
};

std::string StringPrinter::visit_decl(const lox::Decl* decl) {
	return std::visit(
	    overloaded{[this](const lox::TopLevelStmt& tls) {
		               return this->visit_stmt(tls.stmt.get());
	               },
	               [this](const lox::VarDecl& vd) {
		               return std::format(
		                   "(decl {})", this->visit_expr(vd.initializer.get()));
		               //: return this->visit_stmt(vd.initializer.get());
	               },

	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    decl->value);
}

std::string StringPrinter::visit_stmt(const lox::Stmt* stmt) {
	return std::visit(
	    overloaded{[this](const lox::PrintStmt& ps) {
		               return std::format("(print {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [this](const lox::ExprStmt& ps) {
		               return std::format("(expr {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [](const lox::IncStmt& s) {
		               return std::format("(++ {})", s.name);
	               },
	               [](const lox::DecStmt& s) {
		               return std::format("(-- {})", s.name);
	               },
	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    stmt->value);
}

