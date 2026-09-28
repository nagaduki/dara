/* error.cpp */
#include "error.hpp"

#include <iomanip>
#include <sstream>

#include "source.hpp"

#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"

namespace dara::error {

std::string SyntaxError::to_string(const dara::core::Source& src) const {
	auto [l_line, l_col] = src.get_line_col(this->span.start);
	auto [r_line, r_col] = src.get_line_col(this->span.end);

	std::ostringstream oss;
	// oss << "[line " << line << ", col " << col << "] " << message;
	oss << "[line " << l_line << ", col " << l_col << "] " << message
	    << std::endl;
	oss << "[line " << r_line << ", col " << r_col << "] " << message
	    << std::endl;

	// 値が存在しない（std::nullopt）なら EOF 扱い
	if (!actual.has_value()) {
		oss << " (at EOF)";
	} else {
		oss << ": '" << *actual << "'";
	}
	return oss.str();
};

/*
std::string InterpreterError::to_string(const dara::core::Source& src) const {
    auto [l_line, l_col] = src.get_line_col(this->span.start);
    auto [r_line, r_col] = src.get_line_col(this->span.end);

    std::ostringstream oss;
    oss << "[line " << l_line << ", col " << l_col << "] " << message
        << std::endl;
    oss << "[line " << r_line << ", col " << r_col << "] " << message
        << std::endl;

    // 値が存在しない（std::nullopt）なら EOF 扱い
    //
    //if (!actual.has_value()) {
    //    oss << " (at EOF)";
    //} else {
    //    oss << ": '" << *actual << "'";
    //}


    return oss.str();
};
*/

/*
std::string InterpreterError::to_string(const dara::core::Source& src) const {
    auto [l_line, l_col] = src.get_line_col(this->span.start);
    auto [r_line, r_col] = src.get_line_col(this->span.end);
    std::ostringstream oss;
    oss << "Interpreter Error: " << message << "\n";
    oss << " --> [line " << l_line << ", col " << l_col << "] ... [line " <<
r_line << ", col " << r_col << "]\n";

    std::string source_line = src.get_line_string(l_line);
    if (!source_line.empty()) {
        oss << "     |\n";
        oss << std::setw(4) << l_line << " | " << source_line << "\n";
        oss << "     | ";

        for (int i = 1; i < l_col; ++i) {
            oss << " ";
        }

        int length = std::max(1, static_cast<int>(this->span.end -
this->span.start));

        for (int i = 1; i < length; ++i) {
            oss << "^";
        }
        oss << "\n";
    }
    return oss.str();
};
*/

std::string InterpreterError::to_string(const dara::core::Source& src) const {
	// auto [line, col] = src.get_line_col(this->span.start);
	auto [l_line, l_col] = src.get_line_col(this->span.start);
	auto [r_line, r_col] = src.get_line_col(this->span.end);
	std::ostringstream oss;

	// 1. エラーメッセージ自体を 赤色・太字 にして目立たせる！
	oss << COLOR_BOLD << COLOR_RED << "Interpreter Error: " << COLOR_RESET
	    << COLOR_BOLD << message << COLOR_RESET << "\n";
	oss << "  --> [line " << l_line << ", col " << l_col << "] ... [line "
	    << r_line << ", col " << r_col << "]\n";
	;

	std::string source_line = src.get_line_string(l_line);
	if (!source_line.empty()) {
		oss << "   |\n";
		oss << std::setw(2) << l_line << " | " << source_line << "\n";
		oss << "   | ";

		/*
		for (int i = 1; i < l_col; ++i) {
		    oss << " ";
		}
		*/
		for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
			if (source_line[i] == '\t') {
				oss << '\t';
			} else {
				oss << ' ';
			}
		}

		// 2. 波線（Squiggles）を 赤色 で引く！
		int length =
		    std::max(1, static_cast<int>(this->span.end - this->span.start));
		oss << COLOR_RED;  // 波線の色を赤にする
		for (int i = 0; i < length; ++i) {
			oss << "^";
		}

		/*
		  for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
		      if (source_line[i] == '\t') {
		          oss << '\t';
		      } else {
		          oss << ' ';
		      }
		    }
		*/

		oss << COLOR_RESET << "\n";  // 色を戻す
	}
	return oss.str();
}

std::string FatalSyntaxError::to_string(const dara::core::Source& src) const {
	auto [line, col] = src.get_line_col(this->span.start);

	std::ostringstream oss;
	oss << "FATAL: [line " << line << ", col " << col << "] " << what();

	// 値が存在しない（std::nullopt）なら EOF 扱い
	if (!actual.has_value()) {
		oss << " (at EOF)";
	} else {
		oss << ": '" << *actual << "'";
	}
	return oss.str();
};

}  // namespace dara::error
