// value.cpp
#pragma once
#include <memory>
#include <string>
#include <variant>

#include "callable.hpp"
// #include "lexer.hpp"
#include "value.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// using EvalValue = std::variant<char, int, std::string>;

namespace lox::backend {

std::string to_string(const lox::Value& val) {
	return std::visit(
	    overloaded{[](int v) { return std::to_string(v); },
	               [](const std::string& v) { return v; },
	               [](char v) { return std::string(1, v); },
	               [](bool v) {
		               if (v) {
			               return std::string("true");
		               } else {
			               return std::string("false");
		               }
	               },
	               [](std::monostate) { return std::string("nil"); },
	               [](const std::vector<lox::Value>& arr) -> std::string {
		               std::string s = "[";
		               for (size_t i = 0; i < arr.size(); ++i) {
			               s += backend::to_string(arr[i]);  //(*)
			               if (i != arr.size() - 1) {
				               s += ", ";
			               }
		               }
		               s += "]";
		               return s;
	               },
	               [](std::shared_ptr<lox::backend::Callable> c) {
		               return c->to_string();
	               },
	               [](const lox::Range& r) {
		               return std::to_string(r.start) + ".." +
		                      std::to_string(r.end);
	               },
	               [](const auto&) { return std::string("unknown"); }},
	    val.data);
}

}  // namespace lox::backend
