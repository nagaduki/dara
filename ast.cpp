//#pragma once
#include "ast.hpp"

#include "printer.hpp"

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string dara::Expr::to_string() const {
//std::string dara::Expr::to_string() {
	StringPrinter printer;
	return printer.print(this);
}

/*
std::string Value::to_string() const {
	return std::visit(overloaded{[](int v) { return std::to_string(v); },
	                             [](const std::string& v) { return v; }},
	                  value);
}
*/
/*
std::string Value::to_string() const {
	return std::visit(overloaded{[](int v) { return std::to_string(v); },
	                             [](const std::string& v) { return v; }},
	                  data);
}
*/
