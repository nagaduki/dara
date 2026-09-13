/* parser_expr.cpp */

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

using namespace dara::combinator;
using namespace dara::lexer;

// 現在のファイル名と行番号を出力するマクロ
#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

/*
template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
*/

namespace dara::frontend {
// namespace {

static constexpr InfixRule infix_rules[] = {
    InfixRule{InfixOperator::Add,
              {60, 61,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Add, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Sub,
              {60, 61,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Sub, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Mul,
              {70, 71,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Mul, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Div,
              {70, 71,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Div, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Pow,
              {95, 96,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Pow, std::move(l),
	                                  std::move(r)};
               }}},
    // --- 比較演算 ---
    //
    //
    InfixRule{InfixOperator::Is,
              {50, 51,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Is, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::StrictEqual,
              {40, 41,  //  == や != と同じ強さ！
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::StrictEqual, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::EqualEqual,
              {40, 41,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::EqualEqual, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::NotEqual,
              {40, 41,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::NotEqual, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Less,
              {50, 51,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Less, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::LessEqual,
              {50, 51,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::LessEqual, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::Greater,
              {50, 51,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Greater, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::GreaterEqual,
              {50, 51,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::GreaterEqual, std::move(l),
	                                  std::move(r)};
               }}},
    InfixRule{InfixOperator::And,
              {30, 31,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return LogicalOpExpr{LogicalOperator::And, std::move(l),
	                                    std::move(r)};
               }}},
    InfixRule{InfixOperator::Or,
              {20, 21,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return LogicalOpExpr{LogicalOperator::Or, std::move(l),
	                                    std::move(r)};
               }}},
    InfixRule{InfixOperator::Range,
              {10, 11,
               [](ExprPtr l, ExprPtr r) -> ExprValue {
	               return InfixOpExpr{InfixOperator::Range, std::move(l),
	                                  std::move(r)};
               }}},

};

// enum class PrefixOperator { Neg, Pos, Not, Inc, Dec };

static constexpr PrefixRule prefix_rules[] = {
    PrefixRule{PrefixOperator::Neg,
               {85,
                [](ExprPtr r) -> ExprValue {
	                return PrefixOpExpr{PrefixOperator::Neg, std::move(r)};
                }}},
    PrefixRule{PrefixOperator::Pos,
               {85,
                [](ExprPtr r) -> ExprValue {
	                return PrefixOpExpr{PrefixOperator::Pos, std::move(r)};
                }}},
    PrefixRule{PrefixOperator::Not,
               {85,
                [](ExprPtr r) -> ExprValue {
	                return PrefixOpExpr{PrefixOperator::Not, std::move(r)};
                }}},

    /*
    PrefixRule{PrefixOperator::Inc,
               {85,
                [](ExprPtr r) -> ExprValue {
                    return PrefixOpExpr{PrefixOperator::Inc, std::move(r)};
                }}},
    PrefixRule{PrefixOperator::Dec,
               {85,
                [](ExprPtr r) -> ExprValue {
                    return PrefixOpExpr{PrefixOperator::Dec, std::move(r)};
                }}},
    */
};

/* enum class PostfixOperator { Inc, Dec, Fac }; */
static constexpr PostfixRule postfix_rules[] = {
    /*
    PostfixRule{PostfixOperator::Inc,
               {90,
                [](ExprPtr l) -> ExprValue {
                    return PostfixOpExpr{PostfixOperator::Inc, std::move(l)};
                }}},
    PostfixRule{PostfixOperator::Dec,
               {90,
                [](ExprPtr l) -> ExprValue {
                    return PostfixOpExpr{PostfixOperator::Dec, std::move(l)};
                }}},
    */
    PostfixRule{PostfixOperator::Fac,
                {90,
                 [](ExprPtr l) -> ExprValue {
	                 return PostfixOpExpr{PostfixOperator::Fac, std::move(l)};
                 }}},
};

static constexpr MixfixRule mixfix_rules[] = {
    {MixfixOperator::Call, {100}},     // 関数呼び出し ()
    {MixfixOperator::Index, {100}},    // 配列アクセス []
    {MixfixOperator::Property, {100}}  // クラスプロパティ .
};

constexpr const InfixTrait* get_infix_trait(InfixOperator op) {
	for (const auto& rule : infix_rules) {
		if (rule.op == op) {
			return &rule.trait;
		}
	}
	return nullptr;
}

constexpr const PrefixTrait* get_prefix_trait(PrefixOperator op) {
	for (const auto& rule : prefix_rules) {
		if (rule.op == op) {
			return &rule.trait;
		}
	}
	return nullptr;
}

constexpr const PostfixTrait* get_postfix_trait(PostfixOperator op) {
	for (const auto& rule : postfix_rules) {
		if (rule.op == op) {
			return &rule.trait;
		}
	}
	return nullptr;
}

// get_infix_trait と同じように取得関数を用意する
constexpr const MixfixTrait* get_mixfix_trait(MixfixOperator op) {
	for (const auto& rule : mixfix_rules) {
		if (rule.op == op) return &rule.trait;
	}
	return nullptr;
}

std::pair<int, int> Parser::infix_binding_power(InfixOperator op) {
	switch (op) {
			// case InfixOperator::Assign:
			//	return std::pair{2, 1};

		case InfixOperator::EqualEqual:
		case InfixOperator::NotEqual:
			return std::pair{40, 41};

		case InfixOperator::Less:
		case InfixOperator::LessEqual:
		case InfixOperator::Greater:
		case InfixOperator::GreaterEqual:
			return std::pair{50, 51};

		case InfixOperator::Add:
		case InfixOperator::Sub:
			return std::pair{60, 61};
		case InfixOperator::Mul:
		case InfixOperator::Div:
			return std::pair{70, 71};
		case InfixOperator::Pow:
			return std::pair{80, 79};

		default:
			std::unreachable();
	}
}

/* prefix_binding_power */
int Parser::prefix_binding_power(PrefixOperator op) {
	switch (op) {
		case PrefixOperator::Neg:
		case PrefixOperator::Pos:
		case PrefixOperator::Not:
			return 85;
		/*
		case PrefixOperator::Inc:
		case PrefixOperator::Dec:
			return 85;
		*/
		default:
			std::unreachable();
	}
}

/* postfix_binding_power */
int Parser::postfix_binding_power(PostfixOperator op) {
	switch (op) {
		/*
		case PostfixOperator::Inc:
		case PostfixOperator::Dec:
		*/
		case PostfixOperator::Fac:
			return 90;
		default:
			std::unreachable();
	}
}

std::expected<InfixOperator, dara::error::SynataxError> Parser::infix_op()
// Rule<InfixOperator> Parser::infix_op()
// auto Parser::infix_op()
//-> std::expected<InfixOperator, dara::error::SynataxError>
{
	// std::expected<InfixOperator, dara::error::SynataxError> Parser::infix_op() {
	/*
	Source backup = *(this->s);
	if (sym("++")(this->s) || sym("--")(this->s)) {
	    *(this->s) = backup;
	    return std::unexpected(
	        this->s->make_error("++ and -- are not infix operators"));  //(***)
	}
	*/

	// statement
	Source backup = *(this->s);
	if (sym("++")(this->s) || sym("--")(this->s) || sym("+=")(this->s) ||
	    sym("-=")(this->s) || sym("*=")(this->s) || sym("/=")(this->s)) {
		*(this->s) = backup;
		return std::unexpected(
		    this->s->make_error("++ and -- are not infix operators"));  //(***)
	}

	// auto res = (sym('+') || sym('-') || sym('*') || sym('/') || sym('^') ||
	//             sym('=') || sym('<') || sym('>') )(this->s);
	auto res =
	    (sym("<=") || sym(">=") || StrictEqual || sym("==") || sym("!=") ||
	     Is || And || Or || sym("..") || sym("+") || sym("-") || sym("*") ||
	     sym("/") || sym("^") || sym("=") || sym("<") || sym(">"))(this->s);
	// auto res = (Add || Sub || Mul || Div)(s);
	if (!res) {
		return std::unexpected(res.error());
	}
	// char op = res.value();
	std::string op = res.value();

	if (op == "+") return InfixOperator::Add;
	if (op == "-") return InfixOperator::Sub;
	if (op == "*") return InfixOperator::Mul;
	if (op == "/") return InfixOperator::Div;
	if (op == "^") return InfixOperator::Pow;
	if (op == "<") return InfixOperator::Less;
	if (op == ">") return InfixOperator::Greater;
	if (op == "<=") return InfixOperator::LessEqual;
	if (op == ">=") return InfixOperator::GreaterEqual;
	if (op == "==") return InfixOperator::EqualEqual;
	if (op == "===") return InfixOperator::StrictEqual;
	if (op == "!=") return InfixOperator::NotEqual;
	if (op == "and") return InfixOperator::And;
	if (op == "or") return InfixOperator::Or;
	if (op == "is") return InfixOperator::Is;
	if (op == "..") return InfixOperator::Range;
	//	if (op == '=') return InfixOperator::Assign;

	*(this->s) = backup;

	return std::unexpected(this->s->make_error("unknown infix operator"));
}

std::expected<PrefixOperator, dara::error::SynataxError> Parser::prefix_op() {
	Source backup = *(this->s);

	// statement
	if (sym("++")(this->s) || sym("--")(this->s)) {
		*(this->s) = backup;
		return std::unexpected(
		    this->s->make_error("++ and -- are not prefix operators"));
	}

	// auto res = (sym("++") || sym("--") || sym("+") || sym("-") ||
	// sym("!"))(s);
	auto res = (sym("+") || sym("-") || sym("!"))(s);
	if (!res) {
		return std::unexpected(res.error());
	}
	// char op = res.value();
	std::string op = res.value();
	/*
	if (op == "++") return PrefixOperator::Inc;
	if (op == "--") return PrefixOperator::Dec;
	*/
	// if (op == "!") return PrefixOperator::Not;
	if (op == "!") {
		// 🌟 ここを追加
		if (auto peek_res = this->s->peek()) {
			if (peek_res.value() == '=') {
				*(this->s) = backup;
				return std::unexpected(
				    this->s->make_error("this is '!=', not prefix '!'"));
			}
		}
		return PrefixOperator::Not;
	}

	if (op == "-") return PrefixOperator::Neg;
	if (op == "+") return PrefixOperator::Pos;

	*s = backup;

	return std::unexpected(this->s->make_error("unknown prefix operator"));
}

/*
std::expected<PostfixOperator, dara::error::SynataxError> Parser::postfix_op() {
    Source backup = *(this->s);
    // auto res = (string1("++") || string1("--") || string1("!"))(this->s);
    // statement
    if (sym("++")(this->s) || sym("--")(this->s)) {
        *(this->s) = backup;
        return std::unexpected(
            this->s->make_error("++ and -- are not infix operators"));
    }

    auto res = (string1("!"))(this->s);
    if (!res) {
        return std::unexpected(res.error());
    }
    std::string op = res.value();
    if (op == "!") return PostfixOperator::Fac;

    *s = backup;

    return std::unexpected(this->s->make_error("unknown postfix operator"));
}
*/
std::expected<PostfixOperator, dara::error::SynataxError> Parser::postfix_op() {
	Source backup = *(this->s);
	// auto res = (string1("++") || string1("--") || string1("!"))(this->s);
	// statement
	if (sym("++")(this->s) || sym("--")(this->s)) {
		*(this->s) = backup;
		return std::unexpected(
		    this->s->make_error("++ and -- are not infix operators"));
	}

	auto res = (string1("!"))(this->s);
	if (!res) {
		return std::unexpected(res.error());
	}
	std::string op = res.value();
	// if (op == "!") return PostfixOperator::Fac;

	if (op == "!") {
		//  ここを追加：直後の文字が '=' なら、これは '!='
		// なので巻き戻して諦める！
		if (auto peek_res = this->s->peek()) {
			if (peek_res.value() == '=') {
				*(this->s) = backup;
				return std::unexpected(
				    this->s->make_error("this is '!=', not postfix '!'"));
			}
		}
		return PostfixOperator::Fac;  // '=' でなければ無事に階乗として処理
	}

	*s = backup;

	return std::unexpected(this->s->make_error("unknown postfix operator"));
}

std::expected<MixfixOperator, dara::error::SynataxError> Parser::mixfix_op() {
	Source backup = *(this->s);

	// if (sym("(")(this->s)) {
	if (LParen(this->s)) {
		return MixfixOperator::Call;
	}

	if (LBracket(this->s)) {
		return MixfixOperator::Index;
	}

	if (Dot(this->s)) {
		return MixfixOperator::Property;
	}

	*(this->s) = backup;

	return std::unexpected(this->s->make_error("unknown mixfix operator"));
}

std::unique_ptr<Expr> Parser::make_atom(int value) {
	// int line = this->s->line;
	// int col = this->s->col;

	return std::make_unique<Expr>(Expr{
	    .value = IntExpr{value},
	    //.line = line,
	    //.col = col,
	});
}

std::unique_ptr<Expr> Parser::make_atom(double value) {

	return std::make_unique<Expr>(Expr{
	    .value = DoubleExpr{value},
	});
}


std::unique_ptr<Expr> Parser::make_atom(char value) {
	return std::make_unique<Expr>(Expr{CharExpr{value}});
}

/* make_atom for std::string */
std::unique_ptr<Expr> Parser::make_atom(std::string value) {
	return std::make_unique<Expr>(Expr{StringExpr{value}});
}

/* make_atom for bool */
std::unique_ptr<Expr> Parser::make_atom(bool value) {
	return std::make_unique<Expr>(Expr{
	    BoolExpr{value},
	    // line, col の情報も必要であればここに付与
	});
}

std::unique_ptr<Expr> Parser::make_cons(PrefixOperator op,
                                             std::unique_ptr<Expr> rhs) {
	return std::make_unique<Expr>(
	    Expr{PrefixOpExpr{op, std::move(rhs)}});
}

std::unique_ptr<Expr> Parser::make_cons(InfixOperator op,
                                             std::unique_ptr<Expr> lhs,
                                             std::unique_ptr<Expr> rhs) {
	return std::make_unique<Expr>(
	    Expr{InfixOpExpr{op, std::move(lhs), std::move(rhs)}});
}

std::unique_ptr<Expr> Parser::make_cons(PostfixOperator op,
                                             std::unique_ptr<Expr> lhs) {
	return std::make_unique<Expr>(
	    Expr{PostfixOpExpr{op, std::move(lhs)}});
}

/* for AssignExpr */
/*
std::unique_ptr<dara::Expr> make_cons(std::string name,
                                     std::unique_ptr<dara::Expr> value) {
    return std::make_unique<dara::Expr>(
        dara::Expr{dara::AssignExpr{name, std::move(value)}});
}
*/

/* for assign expression */
/*
std::expected<std::unique_ptr<dara::Expr>, dara::error::SynataxError> apply_infix(
    InfixOperator op, std::unique_ptr<dara::Expr> lhs,
    std::unique_ptr<dara::Expr> rhs, Source* s) {
    if (op == InfixOperator::Assign) {
        if (std::holds_alternative<dara::VarExpr>(lhs->value)) {
            auto& var_expr = std::get<dara::VarExpr>(lhs->value);
            return std::make_unique<dara::Expr>(dara::Expr{
                .value = dara::AssignExpr{.name = std::move(var_expr.name),
                                         .value = std::move(rhs)},
                .line = lhs->line,
                .col = lhs->col});
        } else {
            return std::unexpected(s->make_error("Invalid assignment target."));
        }
    } else {
        // new_ilhs = make_cons(infix_op, std::move(lhs), std::move(rhs));
        return std::make_unique<dara::Expr>(
            dara::Expr{dara::InfixOpExpr{op, std::move(lhs), std::move(rhs)}});
    }
}
*/

/*
const InfixTrait* trait_ptr = get_infix_trait(infix_op);
        if (!trait_ptr) {
            *(this->s) = loop_backup;
            break;
        }
 */

/* in parser_expr.cpp */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::call_expr(
    std::unique_ptr<Expr> callee) {
	std::vector<std::unique_ptr<Expr>> arguments;

	// if (sym(")")(this->s)) {
	if (RParen(this->s)) {
		//
		auto call_res = std::make_unique<Expr>();
		call_res->value = CallExpr{std::move(callee), std::move(arguments)};
		return call_res;
	}

	while (true) {
		//
		auto arg_res = this->expr(0);  //

		if (!arg_res) {
			return std::unexpected(arg_res.error());
		}

		arguments.push_back(std::move(arg_res.value()));

		// if (!sym(",")(this->s)) {
		if (!Comma(this->s)) {
			break;
		}
	}

	// if (sym(")")(this->s)) {
	if (!RParen(this->s)) {
		//
		PRINT_LINE();
		return std::unexpected(
		    this->s->make_error("Expected ')' after arguments"));
	}

	auto call_res = std::make_unique<Expr>();
	call_res->value = CallExpr{//
	                           std::move(callee), std::move(arguments)};

	return call_res;
}

/* in parser_expr.cpp */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::index_expr(
    std::unique_ptr<Expr> left) {
	auto index_res = this->expr();
	if (!index_res) {
		return std::unexpected(index_res.error());
	}

	if (!RBracket(this->s)) {
		return std::unexpected(
		    this->s->make_error("epected ']' after array index"));
	}

	auto value_res = std::make_unique<Expr>();
	value_res->value = IndexExpr{std::move(left), std::move(index_res.value())};
	return value_res;
	// return std::make_unique(dara::Expr{
	//     .value = dara::IndexExpr{.array = std::move(left),
	//                             .index = std::move(index_res.value())}});
}

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::prefix_expr() {
	// Source backup = *(this->s);
	// auto op_res = prefix_op(this->s);
	auto op_res = this->prefix_op();
	if (!op_res) return std::unexpected(op_res.error());

	PrefixOperator op = op_res.value();
	const PrefixTrait* trait_ptr = get_prefix_trait(op);
	if (!trait_ptr) {
		return std::unexpected(dara::error::SynataxError{"Unknown prefix operator."});
	}

	// auto rhs_res = this->expr(trait_ptr->rbp);
	// if (!rhs_res) return std::unexpected(rhs_res.error());
	// return make_cons(op, std::move(rhs_res.value()));

	return this->expr(trait_ptr->rbp).transform([&](auto rhs) {
		auto new_expr = std::make_unique<Expr>();
		new_expr->value = trait_ptr->make(std::move(rhs));
		return new_expr;
	});
}

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::mixfix_expr(
    MixfixOperator op, std::unique_ptr<Expr> lhs) {
	switch (op) {
		case MixfixOperator::Call:
			return this->call_expr(std::move(lhs));
		case MixfixOperator::Index:
			return this->index_expr(std::move(lhs));
		case MixfixOperator::Property:
			return this->dot_expr(std::move(lhs));
			// return std::unexpected(
			//     this->s->make_error("property access not implements"));
		default:
			return std::unexpected(
			    this->s->make_error("unknown mixfix operator"));
	}
}
//}

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::paren_expr() {
	// TraceGuard trace("parse_paren_expr");
	//  if (!sym('(')(s)) return std::unexpected(s->make_error("not '('"));
	if (!LParen(this->s)) {
		return std::unexpected(this->s->make_error("not '('"));
	}

	auto inner_res = this->expr(0);
	if (!inner_res) return std::unexpected(inner_res.error());

	// if (!sym(')')(s)) return std::unexpected(s->make_error("expected
	// ')'"));
	if (!RParen(this->s)) {
		return std::unexpected(this->s->make_error("expected ')'"));
	}

	return std::move(inner_res.value());
}

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::bracket_expr() {
	if (!LBracket(this->s)) {
		return std::unexpected(this->s->make_error("not '['"));
	}
	std::vector<std::unique_ptr<Expr>> elements;

	if (!RBracket(this->s)) {
		while (true) {
			auto expr_res = this->expr(0);
			if (!expr_res) {
				return std::unexpected(expr_res.error());
			}

			elements.push_back(std::move(expr_res.value()));

			if (RBracket(this->s)) {
				break;
			}

			if (!Comma(this->s)) {
				return std::unexpected(
				    this->s->make_error("expected ',' or ']'"));
			}
		}
	}
	return std::make_unique<Expr>(
	    Expr{.value = ArrayExpr{std::move(elements)}});
}

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::dot_expr(
    std::unique_ptr<Expr> left) {
	auto name_res = identifier(this->s);
	if (!name_res) {
		return std::unexpected(
		    this->s->make_error("Expected property name after '.'"));
	}

	auto value_res = std::make_unique<Expr>();
	value_res->value = GetExpr{std::move(left), std::move(name_res.value())};

	return value_res;
}

/* atom_expr */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::atom_expr() {
	Source backup = *(this->s);
	int line = this->s->line;
	int col = this->s->col;

	if (auto bool_res = boolean_literal(this->s)) {
		return std::make_unique<Expr>(Expr{
		    .value = BoolExpr{.value = std::move(bool_res.value())},
		    .line = line,
		    .col = col});
	}
	*s = backup;

	if (auto double_res = double_literal(this->s)) {
		/*
		return std::make_unique<dara::Expr>(
		    dara::Expr{dara::IntExpr{int_res.value()}});
		*/
		return std::make_unique<Expr>(Expr{
		    .value = DoubleExpr{.value = std::move(double_res.value())},
		    .line = line,
		    .col = col});
	}
	*s = backup;

	if (auto int_res = integer_literal(this->s)) {
		/*
		return std::make_unique<dara::Expr>(
		    dara::Expr{dara::IntExpr{int_res.value()}});
		*/
		return std::make_unique<Expr>(Expr{
		    .value = IntExpr{.value = std::move(int_res.value())},
		    .line = line,
		    .col = col});
	}
	*s = backup;

	if (auto str_res = string_literal(this->s)) {
		/*
		return std::make_unique<dara::Expr>(
		    dara::Expr{dara::StringExpr{str_res.value()}});
		*/
		return std::make_unique<Expr>(Expr{
		    .value = StringExpr{.value = std::move(str_res.value())},
		    .line = line,
		    .col = col});
	}
	*s = backup;

	if (auto id_res = identifier(this->s)) {
		return std::make_unique<Expr>(
		    Expr{.value = VarExpr{.name = std::move(id_res.value())},
		              .line = line,
		              .col = col});
	}
	*(this->s) = backup;

	return std::unexpected(
	    this->s->make_error("expected an atom (integer, string or variable)"));
}

/* in parser_expr.cpp */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::fn_expr() {
	Source backup = *s;

	if (!Function(this->s)) {
		*s = backup;
		return std::unexpected(
		    this->s->make_error("Not a function expression"));
	}

	if (!LParen(this->s)) {
		*s = backup;
		PRINT_LINE();
		return std::unexpected(this->s->make_error("Expected '(' after 'fn'"));
	}

	std::vector<std::string> parameters;

	if (!RParen(this->s)) {
		do {
			auto param_res = identifier(this->s);

			if (!param_res) return std::unexpected(param_res.error());

			parameters.push_back(param_res.value());
		} while (Comma(this->s));

		if (!RParen(this->s)) {
			return std::unexpected(
			    this->s->make_error("Expected ')' after paramters"));
		}
	}

	if (!LBrace(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '{' before function body"));
	}

	std::vector<std::unique_ptr<Decl>> body;

	FunctionDepthGuard guard(this->function_depth, this->loop_depth);

	while (!RBrace(this->s) && !this->s->isEnd()) {
		auto decl_res = this->decl();
		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}

		body.push_back(std::move(decl_res.value()));
	}

	auto function_expr = std::make_unique<Expr>();
	function_expr->value = FunctionExpr{std::move(parameters), std::move(body)};

	return function_expr;
}

/*
std::expected<std::unique_ptr<dara::Expr>, dara::error::SynataxError> Parser::this_expr() {
    auto this_expr = std::make_unique<dara::Expr>();
    return this_expr;
}
*/

std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::this_expr() {
	// 1. "this" というキーワードを確実に消費（Consume）する
	if (!This(this->s)) {
		return std::unexpected(this->s->make_error("Expected 'this' keyword"));
	}

	// 2. ASTノードを作成し、中身の variant に ThisExpr をセットする
	auto expr_node = std::make_unique<Expr>();
	expr_node->value =
	    ThisExpr{};  // ※ご自身のAST構造体名に合わせてください

	return expr_node;
}

/* nud in parser_expr.cpp */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::nud() {
	// if (auto res = parse_prefix_expr(s)) return res;
	//

	Source backup = *(this->s);

	if (LBracket(this->s)) {
		*(this->s) = backup;
		return bracket_expr();
	}

	if (This(this->s)) {
		*(this->s) = backup;
		auto res = this_expr();
		return res;
	}

	if (Function(this->s)) {
		*(this->s) = backup;
		auto res = fn_expr();
		/*
		if (!res) {
		    return std::unexpected(res.error());
		}
		*/
		return res;
	}

	// auto res = (sym("+") || sym("-") || sym("!"))(s);
	if ((sym("+") || sym("-") || sym("!"))(this->s)) {
		*(this->s) = backup;
		return prefix_expr();
	}

	if (LParen(this->s)) {
		*(this->s) = backup;
		return paren_expr();
	}

	if (auto res = atom_expr()) {
		return res;
	}

	if (this->s->isEnd()) {
		return std::unexpected(this->s->make_error("unexpected EOF"));
	} else {
		PRINT_LINE();
		if (auto peek_res = this->s->peek()) {  //(*)
			std::cout << "[DEBUG] parse_nud failed at char: '"
			          << peek_res.value() << "'" << std::endl;
		}

		return std::unexpected(
		    this->s->make_error("expected an expression"));  //(*)
	}
}

/* Parser::expr in parser_expr.cpp */
std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> Parser::expr(
    int min_bp /* = 0 */) {
	this->s->skip_whitespace();  //(**)
	auto lhs_res = this->nud();
	if (!lhs_res) return std::unexpected(lhs_res.error());

	auto lhs = std::move(lhs_res.value());

	while (true) {
		Source loop_backup = *s;

		// postfix_op //
		if (auto postfix_op_res = this->postfix_op()) {
			PostfixOperator op = postfix_op_res.value();
			const PostfixTrait* trait_ptr = get_postfix_trait(op);
			if (!trait_ptr) {
				return std::unexpected(
				    dara::error::SynataxError{"Unknown postfix operator."});
			}
			if (trait_ptr->lbp < min_bp) {
				*(this->s) = loop_backup;
				break;
			}

			auto new_expr = std::make_unique<Expr>();
			new_expr->value = trait_ptr->make(std::move(lhs));
			lhs = std::move(new_expr);

			// lhs = make_cons(postfix_op, std::move(lhs));

			continue;
		}
		*s = loop_backup;

		// mixfix_op //
		if (auto mixfix_res = this->mixfix_op()) {
			MixfixOperator op = mixfix_res.value();
			const MixfixTrait* trait_ptr = get_mixfix_trait(op);

			if (trait_ptr->lbp < min_bp) {
				*(this->s) = loop_backup;
			} else {
				// std::expected<std::unique_ptr<dara::Expr>, dara::error::SynataxError>
				//     parsed_res;

				/*
				if (op == MixfixOperator::Call) {
				    parsed_res = this->call_expr(std::move(lhs));
				}

				if (op == MixfixOperator::Index) {
				    parsed_res = this->index_expr(std::move(lhs));
				}
				*/
				auto parsed_res = this->mixfix_expr(op, std::move(lhs));
				if (!parsed_res) return std::unexpected(parsed_res.error());

				lhs = std::move(parsed_res.value());

				continue;
			}
		}
		*s = loop_backup;

		// infix_op //
		auto infix_op_res = this->infix_op();

		if (!infix_op_res) {
			*(this->s) = loop_backup;
			break;
		}
		InfixOperator infix_op = infix_op_res.value();
		// auto [ilbp, irbp] = infix_binding_power(infix_op);
		// auto [ilbp, irbp] = infix_binding_power(infix_op);
		const InfixTrait* trait_ptr = get_infix_trait(infix_op);
		if (!trait_ptr) {
			*(this->s) = loop_backup;
			break;
		}

		// if (ilbp < min_bp) {
		if (trait_ptr->lbp < min_bp) {
			*(this->s) = loop_backup;
			break;
		}

		auto irhs_res = this->expr(trait_ptr->rbp)
		                    .transform([&](std::unique_ptr<Expr> irhs) {
			                    auto new_expr = std::make_unique<Expr>();
			                    new_expr->value = trait_ptr->make(
			                        std::move(lhs), std::move(irhs));

			                    return new_expr;
		                    });

		if (!irhs_res) return std::unexpected(irhs_res.error());

		lhs = std::move(irhs_res.value());
	}

	return lhs;
}

}  // namespace dara::frontend
