/* lexer.hpp */
#pragma once

// #include <memory>
#include "combinator.hpp"

namespace dara::lexer {

using namespace dara::combinator;

constexpr bool isEnd(char ch) { return ch == '\0'; }
// constexpr bool isSpace(char ch) { return ch == '\t' || ch == ' '; }
constexpr bool isSpace(char ch) {
	return ch == '\t' || ch == ' ' || ch == '\r' || ch == '\n';
}
// constexpr bool isSpace(char ch) { return ch == '\t' || ch == ' ' || ch ==
// '\r'; }
constexpr bool isDigit(char ch) { return '0' <= ch && ch <= '9'; }
constexpr bool isUpper(char ch) { return 'A' <= ch && ch <= 'Z'; }
constexpr bool isLower(char ch) { return 'a' <= ch && ch <= 'z'; }
constexpr bool isAlpha(char ch) { return isUpper(ch) || isLower(ch); }
constexpr bool isAlphaNum(char ch) { return isAlpha(ch) || isDigit(ch); }
constexpr bool isLetter(char ch) { return isAlpha(ch) || ch == '_'; }

constexpr bool isAdd(char ch) { return ch == '+'; }
constexpr bool isSub(char ch) { return ch == '-'; }
constexpr bool isMul(char ch) { return ch == '*'; }
constexpr bool isDiv(char ch) { return ch == '/'; }
constexpr bool isPow(char ch) { return ch == '^'; }
constexpr bool isAssign(char ch) { return ch == '='; }

constexpr bool isPos(char ch) { return ch == '+'; }
constexpr bool isNeg(char ch) { return ch == '-'; }
constexpr bool isNot(char ch) { return ch == '!'; }

inline dara::combinator::Rule<char> anyChar = dara::combinator::satisfy([](char) { return true; });
// inline dara::combinator::Rule<char> anyChar = satisfy([](char) { return true; });

inline dara::combinator::Rule<char> atEOF = dara::combinator::satisfy(isEnd) || dara::combinator::left("at EOF");
inline dara::combinator::Rule<char> space = dara::combinator::satisfy(isSpace) || dara::combinator::left("not space");
inline dara::combinator::Rule<char> digit = dara::combinator::satisfy(isDigit) || dara::combinator::left("not digit");
inline dara::combinator::Rule<char> upper = dara::combinator::satisfy(isUpper) || dara::combinator::left("not upper");
inline dara::combinator::Rule<char> lower = dara::combinator::satisfy(isLower) || dara::combinator::left("not lower");
inline dara::combinator::Rule<char> alpha = dara::combinator::satisfy(isAlpha) || dara::combinator::left("not alpha");
inline dara::combinator::Rule<char> alphaNum = dara::combinator::satisfy(isAlphaNum) || dara::combinator::left("not alpha or number");
inline dara::combinator::Rule<char> letter = dara::combinator::satisfy(isLetter) || dara::combinator::left("not letter");
inline dara::combinator::Rule<std::string> spaces = dara::combinator::many(space);

inline dara::combinator::Rule<char> Add = dara::combinator::satisfy(isAdd) || dara::combinator::left("not '+'");
inline dara::combinator::Rule<char> Sub = dara::combinator::satisfy(isSub) || dara::combinator::left("not '-'");
inline dara::combinator::Rule<char> Mul = dara::combinator::satisfy(isMul) || dara::combinator::left("not '*'");
inline dara::combinator::Rule<char> Div = dara::combinator::satisfy(isDiv) || dara::combinator::left("not '/'");
inline dara::combinator::Rule<char> Pow = dara::combinator::satisfy(isPow) || dara::combinator::left("not '^'");

//
// inline dara::combinator::Rule<char> Assign = satisfy(isAssign) || left("not '='");

inline dara::combinator::Rule<char> Pos = dara::combinator::satisfy(isPos) || dara::combinator::left("not '+'");
inline dara::combinator::Rule<char> Neg = dara::combinator::satisfy(isNeg) || dara::combinator::left("not '-'");
inline dara::combinator::Rule<char> Not = dara::combinator::satisfy(isNeg) || dara::combinator::left("not '!'");

// enum class InfixOperator { Add, Sub, Mul, Div, Assign, Pow };
enum class InfixOperator {
	Add,
	Sub,
	Mul,
	Div,
	Assign,
	Pow,
	Less,
	Greater,
	EqualEqual,
	StrictEqual,
	LessEqual,
	GreaterEqual,
	NotEqual,
	And,
	Or,
	Is,
	Range
};
// enum class PrefixOperator { Neg, Pos, Not, Inc, Dec };
enum class PrefixOperator { Neg, Pos, Not };
// enum class PrefixOperator { Neg, Pos, Not, Function };
// enum class PostfixOperator { Inc, Dec, Fac };
enum class PostfixOperator { Fac };
enum class LogicalOperator { And, Or };

// enum class MixOperator { Function, Array, Class };

enum class MixfixOperator { Call, Index, Property };
//enum class dara::lexer::MixfixOperator { Call, Index, Property }; // (***)

bool isChar(char ch, char c);
bool isNotChar(char ch, char c);
dara::combinator::Rule<char> char1(char ch);
dara::combinator::Rule<char> sym(char ch);
dara::combinator::Rule<std::string> sym(std::string str);
dara::combinator::Rule<std::string> string1(const std::string& str);

/*
template <typename T>
dara::combinator::Rule<T> token(const dara::combinator::Rule<T>& p) {
    return p << spaces;
}
*/

template <typename T>
dara::combinator::Rule<T> token(const dara::combinator::Rule<T>& p) {
	return [=](Source* s) -> std::expected<T, dara::error::SynataxError> {
		s->skip_whitespace();

		auto res = p(s);

		if (res) {
			s->skip_whitespace();
		}
        return res;
	};
}

inline dara::combinator::Rule<char> Assign = sym('=') || dara::combinator::left("not '='");
extern dara::combinator::Rule<int> integer_literal;
extern dara::combinator::Rule<double> double_literal;
extern dara::combinator::Rule<std::string> string_literal;
// extern dara::combinator::Rule<std::string> boolean_literal;
extern dara::combinator::Rule<bool> boolean_literal;
extern dara::combinator::Rule<std::monostate> nil_literal;
extern dara::combinator::Rule<std::string> identifier;

/* for stmt */
dara::combinator::Rule<std::string> keyword(const std::string& kw);
inline dara::combinator::Rule<std::string> Let = keyword("let");
inline dara::combinator::Rule<std::string> Print = keyword("print");
inline dara::combinator::Rule<std::string> If = keyword("if");
inline dara::combinator::Rule<std::string> Is = keyword("is");
inline dara::combinator::Rule<std::string> And = keyword("and");
inline dara::combinator::Rule<std::string> Or = keyword("or");
inline dara::combinator::Rule<std::string> Else = keyword("else");
inline dara::combinator::Rule<std::string> While = keyword("while");
inline dara::combinator::Rule<std::string> For = keyword("for");
inline dara::combinator::Rule<std::string> In = keyword("in");
inline dara::combinator::Rule<std::string> Function = keyword("fn");
inline dara::combinator::Rule<std::string> Static = keyword("static");
inline dara::combinator::Rule<std::string> Break = keyword("break");
inline dara::combinator::Rule<std::string> Continue = keyword("continue");

inline dara::combinator::Rule<char> Semicolon = sym(';');
inline dara::combinator::Rule<std::string> True = keyword("true");
inline dara::combinator::Rule<std::string> False = keyword("false");
inline dara::combinator::Rule<std::string> Nil = keyword("nil");

inline dara::combinator::Rule<std::string> DotDot = keyword("..");  //(1)
inline dara::combinator::Rule<std::string> Inc = sym("++");         //(1)
inline dara::combinator::Rule<std::string> Dec = sym("--");         //(2)
inline dara::combinator::Rule<std::string> AddAssign = sym("+=");   //(1)
inline dara::combinator::Rule<std::string> SubAssign = sym("-=");   //(1)
inline dara::combinator::Rule<std::string> Plus = keyword("+");     //(1)
inline dara::combinator::Rule<std::string> Minux = keyword("-");    //(2)
inline dara::combinator::Rule<std::string> Excl = keyword("!");     //(2)
inline dara::combinator::Rule<std::string> Return = keyword("return");
inline dara::combinator::Rule<std::string> This = keyword("this");
inline dara::combinator::Rule<std::string> Class = keyword("class");
inline dara::combinator::Rule<std::string> Extends = keyword("extends");
inline dara::combinator::Rule<std::string> With = keyword("with");
inline dara::combinator::Rule<std::string> Equal = keyword("=");
inline dara::combinator::Rule<std::string> StrictEqual = keyword("===");
// inline dara::combinator::Rule<std::string> PreInc = sym("++"); //(1)
// inline dara::combinator::Rule<std::string> PreDec = sym("--"); //(2)

// 区切り記号
inline dara::combinator::Rule<char> Comma = sym(',');
inline dara::combinator::Rule<char> Dot = sym('.');

// 括弧類
inline dara::combinator::Rule<char> LParen = sym('(');
inline dara::combinator::Rule<char> RParen = sym(')');
inline dara::combinator::Rule<char> LBrace = sym('{');
inline dara::combinator::Rule<char> RBrace = sym('}');
inline dara::combinator::Rule<char> LBracket = sym('[');
inline dara::combinator::Rule<char> RBracket = sym(']');

// リテラル用
inline dara::combinator::Rule<char> DQuote = sym('"');
inline dara::combinator::Rule<char> SQuote = sym('\'');

// inline dara::combinator::Rule<std::string> Functtion = sym("fn"); //(2)

// inline dara::combinator::Rule<std::string> Identifier;
}
