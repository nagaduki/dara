/* value.hpp */
#pragma once

#include <memory>
#include <string>
#include <variant>
// #include "callable.hpp"
#include "lexer.hpp"
//#include "class.hpp"
//#include "instance.hpp"


namespace dara::backend {
    class Callable;
}

namespace dara::runtime {
    class Class;
    class Instance;
}

namespace dara {

namespace backend {
class Callable;
}

struct Value;

std::string to_string(const Value& val);

struct Range {
    int start;
    int end;
	bool operator==(const Range& other) const {
        return (this->start == other.start) && (this->end == other.end);
	}
};

using ValueData = std::variant<char, int, double, std::string, bool, Range,
                               //
                               std::vector<Value>,
                               //
                               std::shared_ptr<dara::backend::Callable>,
                               std::shared_ptr<dara::runtime::Class>,
                               std::shared_ptr<dara::runtime::Instance>,
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
