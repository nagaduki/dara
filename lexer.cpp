/* liexer.cpp */

#include "lexer.hpp"

#include <expected>
#include <memory>

namespace dara::lexer {

using namespace dara::combinator;

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

Rule<char> char1(char ch) {
	return satisfy(isChar, ch) || left(std::format("not char {}", ch));
}

bool isChar(char ch, char c) { return c == ch; }

bool isNotChar(char ch, char c) { return c != ch; }

Rule<char> notchar1(char ch) {
	return satisfy(isNotChar, ch) || left(std::format("char {}", ch));
}

// Rule<char> anyBut(char ch) { return token(notchar1(ch)); }
Rule<char> anyBut(char ch) { return notchar1(ch); }

Rule<char> sym(char ch) { return token(char1(ch)); }
// Rule<char> sym(char ch) { return token(char1(ch)); }
Rule<std::string> sym(std::string str) { return token(string1(str)); }

Rule<std::string> string1(const std::string& str) {
	Rule<std::string> np = [=](Source* s)
	    -> std::expected<std::string, dara::error::SynataxError> {
		Source backup = *s;
		for (char c : str) {
			// char1 (*it)(s);
			if (auto res = char1(c)(s)) {
				// do nothing
			} else {
				*s = backup;
				return std::unexpected(res.error());
			}
		}
		return str;
	};
	return np;
};

Rule<bool> boolean_literal =
    [](Source* s) -> std::expected<bool, dara::error::SynataxError> {
	if (True(s)) return true;
	if (False(s)) return false;
	return std::unexpected(s->make_error("not a boolean literal"));
};

/* boolean_literal fix but "false" is not working */
Rule<bool> _boolean_literal =
    [](Source* s) -> std::expected<bool, dara::error::SynataxError> {
	Source backup = *s;
	auto res_true = True(s);
	if (!res_true) {
		*s = backup;
		// return std::unexpected(res.error());
		return std::unexpected(s->make_error("not a boolean literal"));
	}
	return true;

	auto res_false = False(s);
	if (!res_false) {
		*s = backup;
		return std::unexpected(s->make_error("not a boolean literal"));
	}
	return false;
	// return std::unexpected(s->make_error("not a boolean literal"));
};

Rule<std::monostate> nil_literal =
    [](Source* s) -> std::expected<std::monostate, dara::error::SynataxError> {
	if (Nil(s)) return std::monostate{};
	return std::unexpected(s->make_error("not a nil literal"));
};

// boolean_literal
/*
Rule<std::string> _boolean_literal =
    [](Source* s) -> std::expected<std::string, dara::error::SynataxError> {
    Source backup = *s;

    auto boolean_parser = ( True || False )(s);

    auto boolean_res = boolean_parser(s);
    if (!boolean_res) {
        *s = backup;
        // return std::unexpected(res.error());
        return std::unexpected(s->make_error("not boolean"));
    }
    bool str = boolean_res.value();

    return str;
};
*/

// string_literal
Rule<std::string> string_literal =
    [](Source* s) -> std::expected<std::string, dara::error::SynataxError> {
	Source backup = *s;

	// auto string_parser = token(many1(letter));
	// auto string_parser = token(DQuote + many1(letter) + DQuote );
	// auto string_parser = token(DQuote + many1(anyBut('"')) + DQuote );
	// auto string_parser = token(DQuote >> many1(anyBut('"')) << DQuote);
	auto string_parser = token(char1('"') >> many1(anyBut('"')) << char1('"'));

	auto res = string_parser(s);
	if (!res) {
		*s = backup;
		// return std::unexpected(res.error());
		return std::unexpected(s->make_error("not letters"));
	}
	std::string str = res.value();

	return str;
};

Rule<int> integer_literal =
    [](Source* s) -> std::expected<int, dara::error::SynataxError> {
	Source backup = *s;

	auto digits_parser = token(many1(digit));

	return digits_parser(s).and_then(
	    [&](const std::string& str)
	        -> std::expected<int, dara::error::SynataxError> {
		    try {
			    int val = std::stoi(str);
			    return val;
		    } catch (const std::out_of_range& e) {
			    *s = backup;
			    return std::unexpected(s->make_error("number too large"));
		    } catch (const std::invalid_argument& e) {
			    *s = backup;
			    return std::unexpected(s->make_error("invalid number format"));
		    }
	    });
};

Rule<double> double_literal =
    [](Source* s) -> std::expected<double, dara::error::SynataxError> {
	Source backup = *s;

	// auto digits_parser = token(many1(digit));
	auto double_parser = token(many1(digit) + char1('.') + many1(digit));

	return double_parser(s).and_then(
	    [&](const std::string& str)
	        -> std::expected<double, dara::error::SynataxError> {
		    try {
			    double val = std::stod(str);
			    return val;
		    } catch (const std::out_of_range& e) {
			    *s = backup;
			    return std::unexpected(s->make_error("number too large"));
		    } catch (const std::invalid_argument& e) {
			    *s = backup;
			    return std::unexpected(s->make_error("invalid number format"));
		    }
	    });
};

/*
Rule<std::string> identifier =
    token([](Source* s) -> std::expected<std::string, dara::error::SynataxError>
{ return (letter + many(alphaNum))(s);
    });
*/

// identifier
Rule<std::string> identifier =
    [](Source* s) -> std::expected<std::string, dara::error::SynataxError> {
	Source backup = *s;

	// auto identifier_parser = token(many1(alphaNum));
	auto identifier_parser = token(many1(letter) + many(alphaNum));

	auto res = identifier_parser(s);
	if (!res) {
		*s = backup;
		// return std::unexpected(res.error());
		return std::unexpected(s->make_error("not identifier"));
	}
	std::string str = res.value();

	return str;
};

// Rule<char> sym(char ch) { return token(char1(ch)); }

/* for stmt */
Rule<std::string> keyword(const std::string& kw) {
	return [=](Source* s)
	           -> std::expected<std::string, dara::error::SynataxError> {
		s->skip_whitespace();
		Source backup = *s;

		auto res = string1(kw)(s);
		if (!res) {
			*s = backup;
			return std::unexpected(res.error());
		}

		if (!s->isEnd()) {
			auto peek_res = s->peek();
			if (peek_res) {
				char next_ch = peek_res.value();
				if (isLetter(next_ch) || isDigit(next_ch)) {
					*s = backup;
					return std::unexpected(
					    s->make_error("Not a word boundary"));
				}
			}
		}
		// auto space_res = spaces(s);
		s->skip_whitespace();
		return res;
	};
}

}  // namespace dara::lexer
