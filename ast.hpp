#pragma once

#include <memory>
#include <string>
#include <variant>

#include "lexer.hpp"

/* Parser phase */
/* expression */

namespace lox {

class Expr;

struct IntExpr {
	int value;
};
struct CharExpr {
	char value;
};
struct StringExpr {
	std::string value;
};
struct BoolExpr {
	bool value;
};

/* 変数を扱うExpr */
/*
struct AssignExpr {
    std::string name;
};
*/

struct VarExpr {
	std::string name;
	// std::string value;
};

/* mix expr */
struct CallExpr {
	std::unique_ptr<Expr> callee;
	std::vector<std::unique_ptr<Expr>> arguments;
};

struct ArrayExpr {
	std::vector<std::unique_ptr<Expr>> elements;
};

struct IndexExpr {
	std::unique_ptr<Expr> array;
	std::unique_ptr<Expr> index;
};

struct ThisExpr {};

struct GetExpr {
   public:
	std::unique_ptr<Expr> object;
	std::string name;
};

struct MethodDecl {
	std::string name;
	std::unique_ptr<Expr> function;
};

struct ClassDecl {
	std::string name;
	std::unique_ptr<Expr> super;
	std::vector<std::unique_ptr<Expr>> mixins;
	std::vector<MethodDecl> methods;
};

struct Decl;

struct FunctionExpr {
	std::vector<std::string> parameters;
	std::vector<std::unique_ptr<Decl>> body;
};

/*
struct IdentifierExpr {
    std::string name;
};
*/

/* 変数宣言はVarDecl */

/* expression */
/* operator */
struct InfixOpExpr {
	InfixOperator op;
	std::unique_ptr<Expr> lhs;
	std::unique_ptr<Expr> rhs;
};

struct PrefixOpExpr {
	PrefixOperator op;
	std::unique_ptr<Expr> rhs;
};
struct PostfixOpExpr {
	PostfixOperator op;
	std::unique_ptr<Expr> lhs;
};

struct LogicalOpExpr {
	LogicalOperator op;
	std::unique_ptr<Expr> lhs;
	std::unique_ptr<Expr> rhs;
};

enum class MixfixOperator { Call, Index, Property };

struct MixfixTrait {
	int lbp;  // Metafixは右辺(rbp)を持たず、専用の関数で消費するためlbpのみでOK
};

struct MixfixRule {
	MixfixOperator op;
	MixfixTrait trait;
};

// struct MixOpExpr {
//	MixOperator op;
// };

/* node 相当 */
// struct AssignExpr {
//	std::string name;
//	std::unique_ptr<Expr> value;
// };

using ExprValue =
    std::variant<IntExpr, CharExpr, StringExpr, BoolExpr,
                 // IdentifierExpr,
                 // AssignExpr,
                 ArrayExpr, IndexExpr, GetExpr, ThisExpr,
                 //
                 CallExpr, FunctionExpr,
                 // VarExpr, InfixOpExpr, PrefixOpExpr, PostfixOpExpr>;
                 VarExpr, InfixOpExpr, PrefixOpExpr, PostfixOpExpr,
                 LogicalOpExpr>;

class Expr {
   public:
	ExprValue value;
	int line;
	int col;
	std::string to_string() const;  // 実装は .cpp へ
	// std::string to_string();  // 実装は .cpp へ
   private:
};

using ExprPtr = std::unique_ptr<lox::Expr>;
using InfixBuilder = ExprValue(ExprPtr l, ExprPtr r);

struct InfixTrait {
	int lbp;
	int rbp;
	InfixBuilder* make;
};

struct InfixRule {
	InfixOperator op;
	InfixTrait trait;
};

using PrefixBuilder = ExprValue(ExprPtr r);

struct PrefixTrait {
	int rbp;
	PrefixBuilder* make;
};

struct PrefixRule {
	PrefixOperator op;
	PrefixTrait trait;
};

using PostfixBuilder = ExprValue(ExprPtr r);

struct PostfixTrait {
	int lbp;
	PostfixBuilder* make;
};

struct PostfixRule {
	PostfixOperator op;
	PostfixTrait trait;
};

/*
struct ExprStmt {
    std::unique_ptr<Expr> expr;
};
*/

/*
struct NilValue {

}
*/

/* leaf 相当 */
/*
using ValueData = std::variant<char, int, std::string, bool, std::monostate>;
struct Value {
    // ValueData value;
    ValueData data;
    // std::string to_string() const;
    template <typename T>
    bool operator==(const T& other) const {
        if (const T* val = std::get_if<T>(&data)) {
            return *val == other;
        }
        return false;
    }
};
*/
/* statement phase */
struct ExprStmt {
	std::unique_ptr<Expr> expr;
};

/* デバッグ用のprint */
struct PrintStmt {
	std::unique_ptr<Expr> expr;
};

struct AssignStmt {
	std::string name;
	std::unique_ptr<Expr> value;
};

struct SetStmt {
	std::unique_ptr<Expr> object;
	std::string name;
	std::unique_ptr<Expr> value;
};

struct CompoundAssignStmt {
	std::string name;
	InfixOperator op;
	std::unique_ptr<Expr> value;
};

struct CompoundSetStmt {
	std::unique_ptr<Expr> object;
	std::string name;
	InfixOperator op;
	std::unique_ptr<Expr> value;
};

struct Stmt;
struct Decl;

struct BlockStmt {
	// std::vector<std::unique_ptr<Stmt>> statement;
	std::vector<std::unique_ptr<Decl>> declarations;
};

/* Program */
struct Program {
	std::vector<std::unique_ptr<Decl>> declarations;
};

struct IfStmt {
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Stmt> then_branch;
	std::unique_ptr<Stmt> else_branch;
};

struct WhileStmt {
	std::unique_ptr<Expr> condition;
	std::unique_ptr<Stmt> body;
};

struct IncStmt {
	std::string name;
};

struct DecStmt {
	std::string name;
};

struct ReturnStmt {
	std::unique_ptr<Expr> value;
};

struct BreakStmt {};
struct ContinueStmt {};
// struct SwitchStmt {};

struct ForInStmt {
	std::string loop_variable;  // 例: "i"
	std::unique_ptr<Expr> iterable;
	std::unique_ptr<Stmt> body;
};

using StmtValue =
    std::variant<ExprStmt, BlockStmt, IfStmt,
                 //
                 WhileStmt, ForInStmt,
                 //
                 ReturnStmt,
                 // SwtichStmt,
                 IncStmt, DecStmt,
                 //
                 BreakStmt, ContinueStmt,
                 //
                 CompoundAssignStmt, AssignStmt, 
                 //
                 CompoundSetStmt, SetStmt, PrintStmt>;

struct Stmt {
	StmtValue value;
	int line;
	int col;
};

/* declaration phase */

/* Interpreter phase */

template <typename T>
using Result = std::expected<T, InterpreterError>;

/// \brief 変数宣言の文を扱う構造体
/// \details nameとinitializerの２つのmemberをもつ

struct VarDecl {
	std::string name;
	std::unique_ptr<Expr> initializer;
};

struct FuncDecl;
struct ClassDecl;

struct TopLevelStmt {
	std::unique_ptr<Stmt> stmt;
};

/* 今後はIfDecl, WhileDecl, ForDeclと増える */
using DeclValue = std::variant<VarDecl,  // EnumDecl,
                                         // ImportDecl,
                               MethodDecl, ClassDecl,

                               // InterfaxceDecl,
                               // TraitDecl,
                               // TypeAliasDecl,
                               TopLevelStmt>;

struct Decl {
	DeclValue value;
	int line;
	int col;
};

/* statement */
/*
struct IfStmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> then_branch;
    std::unique_ptr<Stmt> else_branch;
};


using StmtValue = std::variant<IfStmt, ReturnStmt>;

struct Stmt {
    StmtValue value;
};

struct ReturnStmt {
    std::unique_ptr<Expr> value;
};

*/

/*
struct BlockStmt {
    std::vector<std::unique_ptr<Stmt>> statement;
};
*/

// std::string to_string(const lox::Value& val);

}  // namespace lox
