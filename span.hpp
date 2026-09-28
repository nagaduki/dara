/* span.hpp */
#pragma once
#include <cstddef>

namespace dara::core {

struct Span {
	size_t start;
	size_t end;

	size_t length() const { return end > start ? end - start : 0; }

	Span merge(const Span& other) const {
		return Span{start < other.start ? start : other.start,
		            end > other.end ? end : other.end};
	}
};

}  // namespace dara::core
