/* lexer.hpp */
#pragma once

// #include <memory>
#include "combinator.hpp"

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

inline Rule<char> anyChar = satisfy([](char) { return true; });
// inline Rule<char> anyChar = satisfy([](char) { return true; });

inline Rule<char> atEOF = satisfy(isEnd) || left("at EOF");
inline Rule<char> space = satisfy(isSpace) || left("not space");
inline Rule<char> digit = satisfy(isDigit) || left("not digit");
inline Rule<char> upper = satisfy(isUpper) || left("not upper");
inline Rule<char> lower = satisfy(isLower) || left("not lower");
inline Rule<char> alpha = satisfy(isAlpha) || left("not alpha");
inline Rule<char> alphaNum = satisfy(isAlphaNum) || left("not alpha or number");
inline Rule<char> letter = satisfy(isLetter) || left("not letter");
inline Rule<std::string> spaces = many(space);

inline Rule<char> Add = satisfy(isAdd) || left("not '+'");
inline Rule<char> Sub = satisfy(isSub) || left("not '-'");
inline Rule<char> Mul = satisfy(isMul) || left("not '*'");
inline Rule<char> Div = satisfy(isDiv) || left("not '/'");
inline Rule<char> Pow = satisfy(isPow) || left("not '^'");

//
// inline Rule<char> Assign = satisfy(isAssign) || left("not '='");

inline Rule<char> Pos = satisfy(isPos) || left("not '+'");
inline Rule<char> Neg = satisfy(isNeg) || left("not '-'");
inline Rule<char> Not = satisfy(isNeg) || left("not '!'");

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

bool isChar(char ch, char c);
bool isNotChar(char ch, char c);
Rule<char> char1(char ch);
Rule<char> sym(char ch);
Rule<std::string> sym(std::string str);
Rule<std::string> string1(const std::string& str);

/*
template <typename T>
Rule<T> token(const Rule<T>& p) {
    return p << spaces;
}
*/

template <typename T>
Rule<T> token(const Rule<T>& p) {
	return [=](Source* s) -> std::expected<T, SyntaxError> {
		s->skip_whitespace();

		auto res = p(s);

		if (res) {
			s->skip_whitespace();
		}
        return res;
	};
}

inline Rule<char> Assign = sym('=') || left("not '='");
extern Rule<int> integer_literal;
extern Rule<double> double_literal;
extern Rule<std::string> string_literal;
// extern Rule<std::string> boolean_literal;
extern Rule<bool> boolean_literal;
extern Rule<std::monostate> nil_literal;
extern Rule<std::string> identifier;

/* for stmt */
Rule<std::string> keyword(const std::string& kw);
inline Rule<std::string> Let = keyword("let");
inline Rule<std::string> Print = keyword("print");
inline Rule<std::string> If = keyword("if");
inline Rule<std::string> Is = keyword("is");
inline Rule<std::string> And = keyword("and");
inline Rule<std::string> Or = keyword("or");
inline Rule<std::string> Else = keyword("else");
inline Rule<std::string> While = keyword("while");
inline Rule<std::string> For = keyword("for");
inline Rule<std::string> In = keyword("in");
inline Rule<std::string> Function = keyword("fn");
inline Rule<std::string> Static = keyword("static");
inline Rule<std::string> Break = keyword("break");
inline Rule<std::string> Continue = keyword("continue");

inline Rule<char> Semicolon = sym(';');
inline Rule<std::string> True = keyword("true");
inline Rule<std::string> False = keyword("false");
inline Rule<std::string> Nil = keyword("nil");

inline Rule<std::string> DotDot = keyword("..");  //(1)
inline Rule<std::string> Inc = sym("++");         //(1)
inline Rule<std::string> Dec = sym("--");         //(2)
inline Rule<std::string> AddAssign = sym("+=");   //(1)
inline Rule<std::string> SubAssign = sym("-=");   //(1)
inline Rule<std::string> Plus = keyword("+");     //(1)
inline Rule<std::string> Minux = keyword("-");    //(2)
inline Rule<std::string> Excl = keyword("!");     //(2)
inline Rule<std::string> Return = keyword("return");
inline Rule<std::string> This = keyword("this");
inline Rule<std::string> Class = keyword("class");
inline Rule<std::string> Extends = keyword("extends");
inline Rule<std::string> With = keyword("with");
inline Rule<std::string> Equal = keyword("=");
inline Rule<std::string> StrictEqual = keyword("===");
// inline Rule<std::string> PreInc = sym("++"); //(1)
// inline Rule<std::string> PreDec = sym("--"); //(2)

// 区切り記号
inline Rule<char> Comma = sym(',');
inline Rule<char> Dot = sym('.');

// 括弧類
inline Rule<char> LParen = sym('(');
inline Rule<char> RParen = sym(')');
inline Rule<char> LBrace = sym('{');
inline Rule<char> RBrace = sym('}');
inline Rule<char> LBracket = sym('[');
inline Rule<char> RBracket = sym(']');

// リテラル用
inline Rule<char> DQuote = sym('"');
inline Rule<char> SQuote = sym('\'');

// inline Rule<std::string> Functtion = sym("fn"); //(2)

// inline Rule<std::string> Identifier;
