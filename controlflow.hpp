/* controlflow.hpp */
#pragma once  // インクルードガードのモダンな書き方
#include <exception>
#include <memory>

#include "value.hpp"

namespace lox::backend {
struct ReturnException : public std::exception {
	lox::Value value;

	explicit ReturnException(lox::Value value) : value(std::move(value)) {}
};

class BreakException : public std::exception {};
class ContinueException : public std::exception {};

}  // namespace lox::backend
