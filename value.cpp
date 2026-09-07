// value.cpp
// #pragma once
#include <memory>
#include <sstream>
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

namespace dara::backend {

std::string to_string(const dara::Value& val) {
	return std::visit(
	    overloaded{[](int v) { return std::to_string(v); },
	               [](double v) {
		               std::ostringstream oss;
		               oss << v;
		               return oss.str();
		               // return std::to_string(v);
	               },
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
	               [](const std::vector<dara::Value>& arr) -> std::string {
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
	               [](std::shared_ptr<dara::backend::Callable> c) {
		               return c->to_string();
	               },
	               [](const dara::Range& r) {
		               return std::to_string(r.start) + ".." +
		                      std::to_string(r.end);
	               },
	               [](const auto&) { return std::string("unknown"); }},
	    val.data);
}

}  // namespace dara::backend
