/* printer */

#include <string>

#include "ast.hpp"
#include "printer.hpp"

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string StringPrinter::print(const dara::Expr* expr) {
	// this->visit_expr(expr);

	return this->visit_expr(expr);
}

std::string StringPrinter::print(const dara::Decl* decl) {
	// this->visit_expr(expr);
	return this->visit_decl(decl);
}

std::string StringPrinter::visit_expr(const dara::Expr* expr) {
	return std::visit(
	    overloaded{[](const dara::IntExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const dara::DoubleExpr& expr) {
		               return std::format("{}", expr.value);
	               },

	               [](const dara::StringExpr& expr) {
		               return std::format("{}", expr.value);
	               },
	               [](const dara::CharExpr& expr) {
		               return std::format("'{}'", expr.value);
	               },
	               [](const dara::VarExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.name);  //(*)
	               },
	               [](const dara::BoolExpr& expr) {  //(*)
		               // return this->visit_stmt(vd.initializer.get());
		               return std::format("\"{}\"", expr.value);  //(*)
	               },
	               [this](const dara::CallExpr& expr) -> std::string {
		               // dummy
		               return "CallExpr";
	               },
	               [this](const dara::ArrayExpr& expr) -> std::string {
		               // dummy
		               return "ArrayExpr";
	               },
	               [this](const dara::IndexExpr& expr) -> std::string {
		               // dummy
		               return "IndexExpr";
	               },
	               [this](const dara::GetExpr& expr) -> std::string {
		               // dummy
		               return "GetExpr";
	               },
	               [this](const dara::ThisExpr& expr) -> std::string {
		               // dummy
		               return "ThisExpr";
	               },

	               [this](const dara::FunctionExpr& e) -> std::string {
		               return "FunctionExpr";
	               },
	               [this](const dara::InfixOpExpr& expr) {
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
	               [this](const dara::LogicalOpExpr& expr) {
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
	               [this](const dara::PrefixOpExpr& expr) {
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
	               [this](const dara::PostfixOpExpr& expr) {
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

std::string StringPrinter::visit_decl(const dara::Decl* decl) {
	return std::visit(
	    overloaded{[this](const dara::TopLevelStmt& tls) {
		               return this->visit_stmt(tls.stmt.get());
	               },
	               [this](const dara::VarDecl& vd) {
		               return std::format(
		                   "(decl {})", this->visit_expr(vd.initializer.get()));
		               //: return this->visit_stmt(vd.initializer.get());
	               },

	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    decl->value);
}

std::string StringPrinter::visit_stmt(const dara::Stmt* stmt) {
	return std::visit(
	    overloaded{[this](const dara::PrintStmt& ps) {
		               return std::format("(print {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [this](const dara::ExprStmt& ps) {
		               return std::format("(expr {})",
		                                  this->visit_expr(ps.expr.get()));
	               },
	               [](const dara::IncStmt& s) {
		               return std::format("(++ {})", s.name);
	               },
	               [](const dara::DecStmt& s) {
		               return std::format("(-- {})", s.name);
	               },
	               [](const auto&) -> std::string {
		               return "unimplemented_decl_node";
	               }},
	    stmt->value);
}
