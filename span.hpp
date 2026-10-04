/* span.hpp */
#pragma once
#include <cstddef>
#include <cstdint>

namespace dara::core {

class Span {
public:
	size_t start;
	size_t end;
    uint16_t id = 0;
	size_t length() const { return end > start ? end - start : 0; }

	Span merge(const Span& other) const {
		return Span{start < other.start ? start : other.start,
		            end > other.end ? end : other.end};
	}
};

}  // namespace dara::core
