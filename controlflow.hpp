/* controlflow.hpp */
#pragma once  // インクルードガードのモダンな書き方
#include <exception>
#include <memory>

#include "value.hpp"

namespace dara::backend {
struct ReturnException : public std::exception {
	dara::Value value;

	explicit ReturnException(dara::Value value) : value(std::move(value)) {}
};

class BreakException : public std::exception {};
class ContinueException : public std::exception {};

}  // namespace dara::backend
