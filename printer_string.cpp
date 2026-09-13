/* printer */

#include <string>

#include "ast.hpp"
#include "printer.hpp"

using namespace dara::lexer;
using namespace dara::ast;

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string StringPrinter::print(const Expr* expr) {
	// this->visit_expr(expr);

	return this->visit_expr(expr);
}

std::string StringPrinter::print(const Decl* decl) {
	// this->visit_expr(expr);
	return this->visit_decl(decl);
}

std::string StringPrinter::visit_expr(const Expr* expr) {
	return std::visit(
	    overloaded{[](const IntExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const DoubleExpr& expr) {
		               return std::format("{}", expr.value);
	               },

	               [](const StringExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const CharExpr& expr) {
		               return std::format("'{}'", expr.value);
	               },
	               [](const VarExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.name);  //(*)
	               },
	               [](const BoolExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.value);  //(*)
	               },
	               [this](const CallExpr& expr) -> std::string {
		               // dummy
		               return "CallExpr";
	               },
	               [this](const ArrayExpr& expr) -> std::string {
		               // dummy
		               return "ArrayExpr";
	               },
	               [this](const IndexExpr& expr) -> std::string {
		               // dummy
		               return "IndexExpr";
	               },
	               [this](const GetExpr& expr) -> std::string {
		               // dummy
		               return "GetExpr";
	               },
	               [this](const ThisExpr& expr) -> std::string {
		               // dummy
		               return "ThisExpr";
	               },

	               [this](const FunctionExpr& e) -> std::string {
		               return "FunctionExpr";
	               },
	               [this](const InfixOpExpr& expr) {
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
	               [this](const LogicalOpExpr& expr) {
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
	               [this](const PrefixOpExpr& expr) {
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
	               [this](const PostfixOpExpr& expr) {
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

std::string StringPrinter::visit_decl(const Decl* decl) {
	return std::visit(
	    overloaded{[this](const TopLevelStmt& tls) {
		               return this->visit_stmt(tls.stmt.get());
	               },
	               [this](const VarDecl& vd) {
		               return std::format(
		                   "(decl {})", this->visit_expr(vd.initializer.get()));
		               //: return this->visit_stmt(vd.initializer.get());
	               },

	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    decl->value);
}

std::string StringPrinter::visit_stmt(const Stmt* stmt) {
	return std::visit(
	    overloaded{[this](const PrintStmt& ps) {
		               return std::format("(print {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [this](const ExprStmt& ps) {
		               return std::format("(expr {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [](const IncStmt& s) {
		               return std::format("(++ {})", s.name);
	               },
	               [](const DecStmt& s) {
		               return std::format("(-- {})", s.name);
	               },
	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    stmt->value);
}
