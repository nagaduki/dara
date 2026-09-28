#pragma once

namespace dara::core {
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
// enum class dara::lexer::MixfixOperator { Call, Index, Property }; // (***)
}

