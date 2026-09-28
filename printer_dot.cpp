/* dotprinter */

#include <iostream>
#include <string>
#include <variant>

#include "ast.hpp"
#include "lexer.hpp"
#include "printer.hpp"

using namespace dara::lexer;
using namespace dara::ast;

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

std::string DotPrinter::print(const Expr* expr) {
	this->out << "digraph AST {" << std::endl;
	this->out << "graph [size=\"8,10!\", dpi=150, nodesep=0.4, ranksep=0.5];"
	          << std::endl;

	this->out << "  node [shape=box, fontname=\"Courier\"];" << std::endl;

	auto id = visit_expr(expr);

	this->out << "}" << std::endl;
	return this->out.str();
};

/* */

std::string DotPrinter::print(
    const std::vector<std::unique_ptr<Decl>>& program) {
	// std::string DotPrinter::print(const dara::Program& program) {
	this->out.str("");
	this->out.clear();
	this->counter = 0;

	this->out << "digraph AST {\n";
	this->out << "  node [shape=box, fontname=\"Helvetica\", style=filled, "
	             "fillcolor=\"#f8f9fa\"];\n";
	this->out << "  edge [color=\"#495057\"];\n\n";

	std::string root_id = this->next_node_id();
	this->out << std::format("node_{} [label=\"Program (REPL Input)\"];\n",
	                         //"node_{} [label=\"Program (REPL Input)\",
	                         // shape=box, " "fillcolor=\"#cce5ff\"];\n",
	                         root_id);

	for (size_t i = 0; i < program.size(); ++i) {
		std::string child_id = this->visit_decl(program[i].get());
		this->out << std::format("node_{}->node_{} [label=\"[{}]\"];\n",
		                         root_id, child_id, i);
	}

	this->out << "}\n";

	return this->out.str();
}

std::string DotPrinter::print(const Program* program) {
	this->out.str("");
	this->out.clear();
	this->counter = 0;

	this->out << "digraph AST {\n";
	this->out << "  node [shape=box, fontname=\"Helvetica\", style=filled, "
	             "fillcolor=\"#f8f9fa\"];\n";
	this->out << "  edge [color=\"#495057\"];\n\n";

	std::string root_id = this->next_node_id();
	this->out << std::format("node_{} [label=\"Program (REPL Input)\"];\n",
	                         root_id);

	for (size_t i = 0; i < program->declarations.size(); ++i) {
		std::string child_id = this->visit_decl(program->declarations[i].get());
		this->out << std::format("node_{}->node_{} [label=\"[{}]\"];\n",
		                         root_id, child_id, i);
	}

	this->out << "}\n";

	return this->out.str();
};

/*
std::string DotPrinter::print(const std::vector<std::unique_ptr<dara::Decl>>
program) { std::stringstream ss; ss << "digraph AST {\n"; ss << "  node
[shape=box, fontname=\"Helvetica\", style=filled, " "fillcolor=\"#f8f9fa\"];\n";
    ss << "  edge [color=\"#495057\"];\n\n";
    ss << " node [shape=box, frontname=\"Helvetica\", style=filled, "
          "fillcolor=\]";

    std::string root_node = "node_program";
    ss << "  " << root_node
       << " [label=\"Program (REPL Input)\", shape=ellipse, "
          "fillcolor=\"#cce5ff\"];\n";

    for (size_t i = 0; i < program.size(); ++i) {
        //std::string child_node_id = this->print_decl(program[i].get(), ss);
        //ss << "  " << root_node << " -> " << child_node_id << " [label=\"[" <<
i
        //   << "]\"];\n";
    }

    ss << "}\n";

    return ss.str();
}
*/

// visit_vecl in printer_dot.cpp
std::string DotPrinter::visit_decl(const Decl* decl) {
	return std::visit(
	    overloaded{
	        [this](const TopLevelStmt& tls) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"TopLevelStmt\", shape=invhouse];",
		                   id)
		            << std::endl;
		        std::string child_id = this->visit_stmt(tls.stmt.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const VarDecl& vd) {
		        std::string id = this->next_node_id();

		        bool is_require = false;
		        std::string module_path = "unknown";

		        if (vd.initializer && std::holds_alternative<CallExpr>(
		                                  vd.initializer->value)) {
			        auto& call_expr =
			            std::get<CallExpr>(vd.initializer->value);

			        if (call_expr.callee &&
			            std::holds_alternative<VarExpr>(
			                call_expr.callee->value)) {
				        auto& var_expr =
				            std::get<VarExpr>(call_expr.callee->value);
				        if (var_expr.name.lexeme == "require") {
					        is_require = true;
					        if (!call_expr.arguments.empty() &&
					            std::holds_alternative<StringExpr>(
					                call_expr.arguments[0]->value)) {
						        module_path = std::get<StringExpr>(
						                          call_expr.arguments[0]->value)
						                          .value;
					        }
				        }
			        }
		        }

		        if (is_require) {
			        this->out << std::format(
			                         "node_{} [label=\"Module({})\", "
			                         "shape=component];",
			                         id, vd.name.lexeme)
			                  << std::endl;
			        return id;
		        } else {
			        this->out << std::format(
			                         "node_{} [label=\"VarDecl({})\", "
			                         "shape=invhouse];",
			                         id, vd.name.lexeme)
			                  << std::endl;
		        }
		        std::string child_id = this->visit_expr(vd.initializer.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const ClassDecl& cd) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"ClassDecl({})\", shape=invhouse];",
		                   id, cd.name.lexeme, id)
		            << std::endl;
		        if (cd.super) {
			        std::string super_id = this->visit_expr(cd.super.get());
			        this->out
			            << std::format("node_{}->node_{} [label=\"super\"];",
			                           id, super_id)
			            << std::endl;
		        }
		        for (size_t i = 0; i < cd.methods.size(); ++i) {
			        std::string method_id =
			            this->visit_expr(cd.methods[i].function.get());  //(**);
			        this->out
			            << std::format(
			                   "node_{}->node_{} [label=\"method({}))\"];", id,
			                   method_id, cd.methods[i].name.lexeme)
			            << std::endl;
		        }
		        return id;
	        },
	        /*
	        [](const auto&) -> std::string {
	            return "unimplemented_decl_node";
	        },
	        */
	        [this](const auto& unknown_decl) -> std::string {
		        // 🚨 this->out に書き込むと、Graphviz の文法エラーになる！
		        // コンソール画面（ターミナル）に直接出力する
		        std::cout << "\n\n======================================\n";
		        std::cout << "[DotPrinter DEBUG] Unknown Decl type:\n";
		        std::cout << typeid(unknown_decl).name() << "\n";
		        std::cout << "======================================\n\n";

		        // dot ファイル上では赤いノードとして描画して目立たせる
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"Unimplemented Decl\", "
		                         "color=\"red\", style=\"filled\", "
		                         "fillcolor=\"#ffcccc\"];",
		                         id)
		                  << std::endl;

		        return id;
	        },
	    },
	    decl->value);
}

// print in printer_dot.cpp
std::string DotPrinter::print(const Decl* decl) {
	this->out << "digraph AST {" << std::endl;
	this->out << "  node [shape=box, fontname=\"Courier\"];" << std::endl;

	std::string root_id = this->next_node_id();
	this->out << std::format("node_{} [label=\"Program\"];", root_id);
	// std::string child_id = this->visit_expr(ps.expr.get());

	auto id = std::visit(
	    overloaded{
	        [this](const TopLevelStmt& tls) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"TopLevelStmt\"];",
		                                 id)
		                  << std::endl;
		        std::string child_id = this->visit_stmt(tls.stmt.get());
		        this->out << std::format("node_{}->node_{}", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        /* next vardecl dot */
	        [this](const VarDecl& vd) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"VarDecl({})\"];", id,
		                                 vd.name.lexeme)
		                  << std::endl;
		        std::string child_id = this->visit_expr(vd.initializer.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;

		        // return this->visit_expr(vd.initializer.get()));
		        //: return this->visit_stmt(vd.initializer.get());
	        },
	        [](const auto&) -> std::string {
		        return "unimplemented_decl_node";
	        }},
	    decl->value);

	this->out << std::format("node_{}->node_{};", root_id, id) << std::endl;
	this->out << "}" << std::endl;

	return this->out.str();
};

std::string DotPrinter::next_node_id() {
	return std::format("{:05}", ++(this->counter));
};

// visit_stmt in printer_dot.cpp
/*
std::string DotPrinter::visit_stmt(const dara::Stmt* stmt) {
    auto id = std::visit(
        overloaded{
            [this](const dara::BlockStmt& bs) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"BlockStmt\"];", id)
<< std::endl; for (size_t i = 0; i < bs.declarations.size(); ++i) { std::string
child_id = this->visit_decl(bs.declarations[i].get()); this->out <<
std::format("node_{}->node_{};", id, child_id) << std::endl;
                }
                return id;
            },
            [this](const dara::IfStmt& is) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"IfStmt\"];", id) <<
std::endl; std::string condition_id = this->visit_expr(is.condition.get());
                this->out << std::format("node_{}->node_{}
[label=\"condition\"];", id, condition_id) << std::endl; std::string then_id =
this->visit_stmt(is.then_branch.get()); this->out <<
std::format("node_{}->node_{} [label=\"then\"];", id, then_id) << std::endl; if
(is.else_branch) { std::string else_id = this->visit_stmt(is.else_branch.get());
                    this->out << std::format("node_{}->node_{};", id, else_id)
<< std::endl;
                }
                return id;
            },
            [this](const dara::WhileStmt& ws) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"WhileStmt\"];", id)
<< std::endl; std::string condition_id = this->visit_expr(ws.condition.get());
                this->out << std::format("node_{}->node_{}
[label=\"condition\"];", id, condition_id) << std::endl; std::string then_id =
this->visit_stmt(ws.body.get()); this->out << std::format("node_{}->node_{}
[label=\"body\"];", id, then_id) << std::endl; return id;
            },
            [this](const dara::ForInStmt& ws) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"ForInStmt\"];", id)
<< std::endl; std::string var_id = this->next_node_id(); this->out <<
std::format("node_{}->node_{};", id, var_id) << std::endl; this->out <<
std::format("node_{} [label=\"loop var: {}\"];", var_id, ws.loop_variable) <<
std::endl; std::string body_id = this->visit_stmt(ws.body.get()); this->out <<
std::format("node_{}->node_{} [label=\"body\"];", id, body_id) << std::endl;
                return id;
            },
            [this](const dara::PrintStmt& ps) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"PrintStmt\"];", id)
<< std::endl; std::string child_id = this->visit_expr(ps.expr.get()); this->out
<< std::format("node_{}->node_{};", id, child_id) << std::endl; return id;
            },
            [this](const dara::AssignStmt& as) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"AssignStmt({})\"];",
id, as.name) << std::endl; std::string child_id =
this->visit_expr(as.value.get()); this->out << std::format("node_{}->node_{};",
id, child_id) << std::endl; return id;
            },
            [this](const dara::ExprStmt& es) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"ExprStmt\"];", id) <<
std::endl; std::string child_id = this->visit_expr(es.expr.get()); this->out <<
std::format("node_{}->node_{};", id, child_id) << std::endl; return id;
            },
            [this](const dara::IncStmt& es) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"IncStmt\"];", id) <<
std::endl; std::string child_id = this->next_node_id(); this->out <<
std::format("node_{} [label=\"var: {}\", shape=house];", child_id, es.name);
                this->out << std::format("node_{}->node_{};", id, child_id) <<
std::endl; return id;
            },
            [this](const dara::DecStmt& es) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"DecStmt\"];", id) <<
std::endl; std::string child_id = this->next_node_id(); this->out <<
std::format("node_{} [label=\"var: {}\", shape=house];", child_id, es.name);
                this->out << std::format("node_{}->node_{};", id, child_id) <<
std::endl; return id;
            },
            [this](const dara::CompoundAssignStmt& stmt) -> std::string {
                std::string op_str;
                switch (stmt.op) {
                    case InfixOperator::Add: op_str = "+="; break;
                    case InfixOperator::Sub: op_str = "-="; break;
                    case InfixOperator::Mul: op_str = "*="; break;
                    case InfixOperator::Div: op_str = "/="; break;
                    default: op_str = "?="; break;
                }
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"CompoundAssign( {}
)\"];", id, op_str) << std::endl; std::string name_id = this->next_node_id();
                this->out << std::format("node_{} [label=\"var: {}\",
shape=house];", name_id, stmt.name) << std::endl; this->out <<
std::format("node_{}->node_{} [label=\"target\"];", id, name_id) << std::endl;
                std::string val_id = this->visit_expr(stmt.value.get());
                this->out << std::format("node_{}->node_{} [label=\"value\"];",
id, val_id) << std::endl; return id;
            },
            [this](const dara::SetStmt& ss) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"SetStmt( .{} )\",
shape=box];", id, ss.name) << std::endl; if (ss.object) { std::string object_id
= this->visit_expr(ss.object.get()); this->out << std::format("node_{}->node_{}
[label=\"object\"];", id, object_id) << std::endl;
                }
                if (ss.value) {
                    std::string value_id = this->visit_expr(ss.value.get());
                    this->out << std::format("node_{}->node_{}
[label=\"value\"];", id, value_id) << std::endl;
                }
                return id;
            },
            [this](const dara::CompoundSetStmt& stmt) -> std::string {
                std::string id = this->next_node_id();
                std::string op_str;
                switch (stmt.op) {
                    case InfixOperator::Add: op_str = "+="; break;
                    case InfixOperator::Sub: op_str = "-="; break;
                    case InfixOperator::Mul: op_str = "*="; break;
                    case InfixOperator::Div: op_str = "/="; break;
                    default: op_str = "?="; break;
                }
                this->out << std::format("node_{} [label=\"CompoundSet( .{} {}
)\", shape=box];", id, stmt.name, op_str) << std::endl; std::string obj_id =
this->visit_expr(stmt.object.get()); this->out << std::format("node_{}->node_{}
[label=\"object\"];", id, obj_id) << std::endl; std::string val_id =
this->visit_expr(stmt.value.get()); this->out << std::format("node_{}->node_{}
[label=\"value\"];", id, val_id) << std::endl; return id;
            },
            [this](const dara::BreakStmt& bs) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"BreakStmt\",
shape=house];", id) << std::endl; return id;
            },
            [this](const dara::ContinueStmt& cs) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"ContinueStmt\",
shape=house];", id) << std::endl; return id;
            },
            [this](const dara::ReturnStmt& rs) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"ReturnStmt\",
shape=house];", id) << std::endl; if (rs.value != nullptr) { std::string id_expr
= this->visit_expr(rs.value.get()); this->out <<
std::format("node_{}->node_{};", id, id_expr) << std::endl;
                }
                return id;
            },
            [this](const auto& s) -> std::string {
                std::string id = this->next_node_id();
                this->out << std::format("node_{} [label=\"UnimplementedStmt\",
color=\"red\", style=\"filled\", fillcolor=\"#ffcccc\"];", id) << std::endl;
                return id;
            }},
        stmt->value);
    return id;
}
*/

// visit_stmt in printer_dot.cpp

std::string DotPrinter::visit_stmt(const Stmt* stmt) {
	auto id = std::visit(
	    overloaded{
	        [this](const BlockStmt& bs) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"BlockStmt\"];", id)
		                  << std::endl;
		        // BlockStmt vector for loop //
		        // std::string child_id = this->visit_expr(bs.expr.get());
		        // //
		        for (size_t i = 0; i < bs.declarations.size(); ++i) {
			        std::string child_id =
			            this->visit_decl(bs.declarations[i].get());
			        this->out << std::format("node_{}->node_{};", id, child_id)
			                  << std::endl;
		        }

		        return id;
	        },
	        [this](const IfStmt& is) {
		        std::string id = this->next_node_id();

		        this->out << std::format("node_{} [label=\"IfStmt\"];", id)
		                  << std::endl;
		        std::string condition_id = this->visit_expr(is.condition.get());
		        this->out << std::format(
		                         "node_{}->node_{} [label=\"condition\"];", id,
		                         condition_id)
		                  << std::endl;
		        std::string then_id = this->visit_stmt(is.then_branch.get());
		        this->out << std::format("node_{}->node_{} [label=\"then\"];",
		                                 id, then_id)
		                  << std::endl;
		        if (is.else_branch) {
			        std::string else_id =
			            this->visit_stmt(is.else_branch.get());
			        this->out << std::format("node_{}->node_{};", id, else_id)
			                  << std::endl;
		        }
		        return id;
	        },
	        [this](const WhileStmt& ws) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"WhileStmt\"];", id)
		                  << std::endl;
		        std::string condition_id = this->visit_expr(ws.condition.get());
		        this->out << std::format(
		                         "node_{}->node_{} [label=\"condition\"];", id,
		                         condition_id)
		                  << std::endl;
		        std::string then_id = this->visit_stmt(ws.body.get());
		        this->out << std::format("node_{}->node_{} [label=\"body\"];",
		                                 id, then_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const ForInStmt& ws) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"ForInStmt\"];", id)
		                  << std::endl;
		        // std::string var_id =
		        // this->visit_expr(ws.loop_variable.get());
		        std::string var_id = this->next_node_id();
		        this->out << std::format("node_{}->node_{};", id, var_id)
		                  << std::endl;
		        this->out << std::format("node_{} [label=\"loop var: {}\"];",
		                                 var_id, ws.loop_variable.lexeme)
		                  << std::endl;
		        std::string body_id = this->visit_stmt(ws.body.get());
		        this->out << std::format("node_{}->node_{} [label=\"body\"];",
		                                 id, body_id)
		                  << std::endl;
		        return id;
	        },

	        [this](const PrintStmt& ps) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"PrintStmt\"];", id)
		                  << std::endl;
		        std::string child_id = this->visit_expr(ps.expr.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const AssignStmt& as) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"AssignStmt({})\"];",
		                                 id, as.name.lexeme)
		                  << std::endl;
		        std::string child_id = this->visit_expr(as.value.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },

	        [this](const ExprStmt& es) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"ExprStmt\"];", id)
		                  << std::endl;
		        std::string child_id = this->visit_expr(es.expr.get());
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const IncStmt& es) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"IncStmt\"];", id)
		                  << std::endl;
		        // std::string child_id = es.name;
		        std::string child_id = this->next_node_id();
		        this->out << std::format(
		            "node_{} [label=\"var: {}\", shape=house];", child_id,
		            es.name.lexeme);
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const DecStmt& es) {
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"DecStmt\"];", id)
		                  << std::endl;
		        std::string child_id = this->next_node_id();
		        this->out << std::format(
		            "node_{} [label=\"var: {}\", shape=house];", child_id,
		            es.name.lexeme);
		        this->out << std::format("node_{}->node_{};", id, child_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const CompoundAssignStmt& stmt) {
		        //
		        std::string op_str;
		        switch (stmt.op) {
			        case InfixOperator::Add:
				        op_str = "+=";
				        break;
			        case InfixOperator::Sub:
				        op_str = "-=";
				        break;
			        case InfixOperator::Mul:
				        op_str = "*=";
				        break;
			        case InfixOperator::Div:
				        op_str = "/=";
				        break;
			        default:
				        op_str = "?=";
				        break;
		        }
		        std::string id = this->next_node_id();
		        this->out << std::format("node_{} [label=\"{}\"];", id, op_str)
		                  << std::endl;

		        // name
		        std::string name_id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"var: {}\" shape=house];",
		                         name_id, stmt.name.lexeme)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{} [label=\"target\"];",
		                                 id, name_id)
		                  << std::endl;
		        // val
		        std::string val_id = this->visit_expr(stmt.value.get());
		        this->out << std::format("node_{}->node_{}[label=\"value\"];",
		                                 id, val_id)
		                  << std::endl;
		        return id;
	        },
	        [this](const SetStmt& ss) {
		        std::string id = this->next_node_id();

		        // ノード自身のラベルにはプロパティ名（name）を含める
		        this->out
		            << std::format(
		                   "node_{} [label=\"SetStmt( .{} )\", shape=box];", id,
		                   ss.name.lexeme)
		            << std::endl;

		        // 1. object（左辺のドットの前。 p や this など）への矢印
		        if (ss.object) {
			        std::string object_id = this->visit_expr(ss.object.get());
			        this->out
			            << std::format("node_{}->node_{} [label=\"object\"];",
			                           id, object_id)
			            << std::endl;
		        }

		        // 2. value（右辺。 代入する値）への矢印
		        if (ss.value) {
			        std::string value_id = this->visit_expr(ss.value.get());
			        this->out
			            << std::format("node_{}->node_{} [label=\"value\"];",
			                           id, value_id)
			            << std::endl;
		        }

		        return id;
	        },

	        [this](const CompoundSetStmt& stmt) {
		        std::string id = this->next_node_id();

		        std::string op_str;

		        switch (stmt.op) {
			        case InfixOperator::Add:
				        op_str = "+=";
				        break;
			        case InfixOperator::Sub:
				        op_str = "-=";
				        break;
			        case InfixOperator::Mul:
				        op_str = "*=";
				        break;
			        case InfixOperator::Div:
				        op_str = "/=";
				        break;
			        default:
				        op_str = "?=";
				        break;
		        }

		        this->out << std::format(
		                         "node_{} [label=\".{} {}\", shape=box];", id,
		                         stmt.name.lexeme, op_str)
		                  << std::endl;
		        // object
		        std::string obj_id = this->visit_expr(stmt.object.get());
		        this->out << std::format("node_{}->node_{} [label=\"object\"];",
		                                 id, obj_id)
		                  << std::endl;
		        // val
		        std::string val_id = this->visit_expr(stmt.value.get());
		        this->out << std::format("node_{}->node_{}[label=\"value\"];",
		                                 id, val_id)
		                  << std::endl;
		        return id;
	        },

	        [this](const BreakStmt& bs) {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"BreakStmt\", shape=house];",
		                         id)

		                  << std::endl;
		        return id;
	        },
	        [this](const ContinueStmt& cs) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"ContinueStmt\", shape=house];", id)
		            << std::endl;
		        return id;
	        },
	        [this](const ReturnStmt& rs) {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"ReturnStmt\", shape=house];",
		                         id)
		                  << std::endl;
		        if (rs.value != nullptr) {
			        // std::string child_id = this->next_node_id();
			        std::string id_expr = this->visit_expr(rs.value.get());
			        this->out << std::format("node_{}->node_{};", id, id_expr)
			                  << std::endl;
		        }

		        return id;
	        },

	        [this](const auto& s) -> std::string {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"UnimplementedStmt\", "
		                         "color=\"red\", style=\"filled\", "
		                         "fillcolor=\"#ffcccc\"];",
		                         id)
		                  << std::endl;
		        // return "unimplemented_decl_node.";
		        return id;
	        }},
	    stmt->value);
	return id;
};

// visit_expr in printer_dot.cpp
std::string DotPrinter::visit_expr(const Expr* expr) {
	return std::visit(
	    // overloaded{[this](const IdentifierExpr& expr) {
	    overloaded{
	        [this](const VarExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"var: {}\", shape=house];",
		                         id, expr.name.lexeme)
		                  << std::endl;
		        return id;
	        },
	        [this](const FunctionExpr& expr) {
		        std::string id = this->next_node_id();
		        std::string params_str = "";
		        for (size_t i = 0; i < expr.parameters.size(); ++i) {
			        params_str += expr.parameters[i].lexeme;
			        if (i < expr.parameters.size() - 1) {
				        params_str += ",";
			        }
		        }
		        this->out
		            << std::format(
		                   "node_{} [label=\"FunctionExpr: {}\", shape=house];",
		                   id, params_str)
		            << std::endl;
		        // parameter
		        std::string id_parameter = this->next_node_id();
		        this->out << std::format("node_{} [label=\"parameter:{}\"];",
		                                 id_parameter, params_str);
		        this->out << std::format("node_{}->node_{};", id, id_parameter)
		                  << std::endl;

		        // body
		        std::string id_body = this->next_node_id();
		        this->out << std::format("node_{} [label=\"body\"];", id_body)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, id_body)
		                  << std::endl;

		        for (const auto& vecl : expr.body) {
			        // auto stmt_res = this->visit_decl(vecl.get());
			        // auto stmt_res = this->visit_decl(vecl.get());
			        // std::string child_id = this->visit_decl(vecl.get());
			        std::string child_id = this->visit_decl(vecl.get());
			        this->out
			            << std::format("node_{}->node_{};", id_body, child_id)
			            << std::endl;
		        }

		        return id;
	        },
	        [this](const CallExpr& expr) {
		        std::string id = this->next_node_id();
		        if (std::holds_alternative<VarExpr>(expr.callee->value)) {
			        auto& var_expr = std::get<VarExpr>(expr.callee->value);
			        if (var_expr.name.lexeme == "require") {
				        this->out << std::format(
				                         "node_{} [label=\"CallExpr\", "
				                         "shape=house];",
				                         id)
				                  << std::endl;
			        }
		        } else {
			        this->out
			            << std::format(
			                   "node_{} [label=\"CallExpr\", shape=house];", id)
			            << std::endl;
		        }
		        // 2. Callee (呼ばれる関数/変数) を再帰的にプリント
		        std::string callee_id = this->visit_expr(expr.callee.get());

		        // ラベルをつけて線を引く
		        this->out << std::format("node_{}->node_{} [label=\"callee\"];",
		                                 id, callee_id)
		                  << std::endl;

		        int arg_index = 0;
		        for (const auto& arg : expr.arguments) {
			        std::string arg_id = this->visit_expr(arg.get());

			        this->out
			            << std::format("node_{}->node_{} [label=\"arg {}\"];",
			                           id, arg_id, arg_index)
			            << std::endl;
			        arg_index++;
		        }
		        return id;
	        },
	        [this](const IntExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"integer: {}\", shape=house];", id,
		                   expr.value)
		            << std::endl;
		        return id;
	        },
	        [this](const DoubleExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"double: {}\", shape=house];", id,
		                   expr.value)
		            << std::endl;
		        return id;
	        },

	        [this](const CharExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"char {}\", shape=house];",
		                         id, expr.value)
		                  << std::endl;
		        return id;
	        },
	        [this](const StringExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"string: {}\", shape=house];",
		                         id, expr.value)
		                  << std::endl;
		        return id;
	        },
	        [this](const BoolExpr& expr) {
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"boolean: {}\", shape=house];", id,
		                   expr.value)
		            << std::endl;
		        return id;
	        },
	        [this](const ArrayExpr& expr) {
		        std::string id = this->next_node_id();

		        // 1. ArrayExpr 自体のノードを出力
		        this->out << std::format(
		                         "node_{} [label=\"ArrayExpr\", shape=house];",
		                         id)
		                  << std::endl;

		        // 2. 配列の各要素（子ノード）を再帰的に出力し、矢印で結ぶ
		        int index = 0;
		        for (const auto& elem : expr.elements) {
			        std::string child_id = this->visit_expr(elem.get());

			        // 矢印のラベルにインデックス番号（[0], [1]...）をつける
			        this->out
			            << std::format("node_{}->node_{} [label=\"[{}]\"];", id,
			                           child_id, index)
			            << std::endl;
			        index++;
		        }

		        return id;
	        },

	        /*
	        [this](const dara::CallExpr& expr) {  // dummy
	            std::string id = this->next_node_id();
	            this->out << std::format(
	                             "node_{} [label=\"CallExpr\", shape=house];",
	                             id)
	                      << std::endl;
	            return id;
	        },
	        */
	        [this](const IndexExpr& expr) {  // dummy
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"IndexExpr\", shape=house];",
		                         id)
		                  << std::endl;
		        return id;
	        },
	        [this](const GetExpr& ge) {  // dummy
		        //dara::PrintStmt();
		        std::string id = this->next_node_id();
		        this->out
		            << std::format(
		                   "node_{} [label=\"GetExpr( .{} )\", shape=box];", id,
		                   ge.name.lexeme)
		            << std::endl;
		        if (ge.object) {
			        std::string object_id = this->visit_expr(ge.object.get());
			        this->out
			            << std::format("node_{}->node_{} [label=\"object\"];",
			                           id, object_id)
			            << std::endl;
		        }
		        return id;
	        },
	        [this](const ThisExpr& expr) {  // dummy
		        std::string id = this->next_node_id();
		        this->out << std::format(
		                         "node_{} [label=\"ThisExpr\", shape=house];",
		                         id)
		                  << std::endl;
		        return id;
	        },

	        [this](const InfixOpExpr& expr) {
		        std::string op_str;
		        switch (expr.op) {
			        case InfixOperator::Add:  //(***)
				        op_str = "+";
				        break;
			        case InfixOperator::Sub:  //(***)
				        op_str = "-";
				        break;
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
			        case InfixOperator::Is:
				        op_str = "is";
				        break;
			        case InfixOperator::StrictEqual:
				        op_str = "===";
				        break;
			        case InfixOperator::NotEqual:
				        op_str = "!=";
				        break;
			        case InfixOperator::Range:
				        op_str = "..";
				        break;
		        }
		        std::string id = this->next_node_id();
		        std::string lid = visit_expr(expr.lhs.get());
		        std::string rid = visit_expr(expr.rhs.get());

		        this->out << std::format("node_{} [label=\"{}\"];", id, op_str)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, lid)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, rid)
		                  << std::endl;

		        return id;
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
		        std::string id = this->next_node_id();
		        std::string rid = visit_expr(expr.rhs.get());
		        this->out << std::format("node_{} [label=\"{}\"];", id, op_str)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, rid)
		                  << std::endl;
		        return id;
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
		        std::string id = this->next_node_id();
		        std::string rid = visit_expr(expr.rhs.get());

		        this->out << std::format("node_{} [label=\"{}\"];", id, op_str)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, rid)
		                  << std::endl;

		        return id;
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
		        std::string id = this->next_node_id();
		        std::string lid = visit_expr(expr.lhs.get());

		        this->out << std::format("node_{} [label=\"{}\"];", id, op_str)
		                  << std::endl;
		        this->out << std::format("node_{}->node_{};", id, lid)
		                  << std::endl;
		        return id;
	        }},

	    expr->value);
};
