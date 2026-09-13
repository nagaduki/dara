/* value.hpp */
#pragma once

#include <memory>
#include <string>
#include <variant>
#include <memory_resource>
// #include "callable.hpp"
#include "lexer.hpp"
//#include "class.hpp"
//#include "instance.hpp"


namespace dara::backend {
    class Callable;
}

namespace dara::backend {
    class Class;
    class Instance;
    class Callable;
}

/*
namespace backend {
class Callable;
}
*/

namespace dara::backend {

struct Value;

using Allocator = std::pmr::polymorphic_allocator<std::byte>;
//using String = std::pmr::basic_string<char>;
using String = std::pmr::string;
using Array = std::pmr::vector<Value>;

std::string to_string(const Value& val);

struct Range {
    int start;
    int end;
	bool operator==(const Range& other) const {
        return (this->start == other.start) && (this->end == other.end);
	}
};

using ValueData = std::variant<char, int, double, bool, Range,
                               //std::string,
                               String,
                               //
                               //std::vector<Value>,
                               Array,
                               //
                               std::shared_ptr<Callable>,
                               std::shared_ptr<Class>,
                               std::shared_ptr<Instance>,
                               //
                               std::monostate>;

struct Value {
	// ValueData value;
	ValueData data;
	// std::string to_string() const;
	bool operator==(const Value& other) const {
		return this->data == other.data;
	}

	template <typename T>
	bool operator==(const T& other) const {
		if (const T* val = std::get_if<T>(&data)) {
			return *val == other;
		}
		return false;
	}

	//std::string to_string() const;
};

}  // namespace dara
