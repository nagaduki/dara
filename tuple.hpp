/* taple.hpp */
#pragma once
#include <chrono>
#include <memory_resource>
#include <set>
#include <expected>

#include "callable.hpp"
#include "class.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "lexer.hpp"
#include "value.hpp"
#include "ast.hpp"

namespace dara::backend {

class Interpreter;

struct Tuple {
	std::pmr::vector<Value> elements;
	explicit Tuple(std::pmr::memory_resource* alloc) : elements(alloc) {}
	explicit Tuple(std::pmr::vector<Value> elements)
	    : elements(std::move(elements)) {}
	size_t size() const noexcept { return elements.size(); }
	bool empty() const noexcept { return elements.empty(); }

	const Value& operator[](size_t index) const { return elements[index]; }

	auto begin() const noexcept { return elements.begin(); }
	auto end() const noexcept { return elements.end(); }

    bool operator==(const Tuple& other) const = default;
};


/*
std::expected<Value, dara::error::InterpreterError> get_tuple_property(
    Interpreter& interpreter,
    const std::shared_ptr<Tuple> tuple,
    const dara::ast::Identifier& name);
*/
std::expected<Value, dara::error::InterpreterError> get_tuple_property(
    Interpreter& interpreter,
    const std::shared_ptr<Tuple> tuple,
    const dara::ast::Identifier& name);



// Tuple
class BuiltinTupleSize : public Callable {
   public:
	BuiltinTupleSize(std::shared_ptr<Tuple> tuple) : tuple(std::move(tuple)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn tuple_size>"; }

   private:
	std::shared_ptr<Tuple> tuple;
};


}  // namespace dara::backend
