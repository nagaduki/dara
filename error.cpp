/* error.cpp */
#include "error.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>

#include "source.hpp"

#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"

namespace dara::error {

static std::string build_error_message(const dara::core::Source& src,
                                       uint32_t start, uint32_t end,
                                       const std::string& error_title,
                                       const std::string& message) {
	auto [l_line, l_col] = src.get_line_col(start);
    auto [r_line, r_col] = src.get_line_col(end);
	std::string filename = src.name;

	// モダン化：絶対パスを「相対パス」に変換
	try {
		std::filesystem::path p(filename);
		if (p.is_absolute()) {
			filename =
			    std::filesystem::relative(p, std::filesystem::current_path())
			        .string();
		}
	} catch (const std::exception&) {
		// 失敗時はフルパスのまま
	}

	std::stringstream ss;
	std::string line_str = std::to_string(l_line);

	// ヘッダー部
	std::string header =
	    std::format("{}:{}:{}: error: {}", filename, l_line, l_col, message);
	std::string prefix = std::format(" {} | ", line_str);
	std::string source_line = src.get_line_string(l_line);

	if (!source_line.empty()) {
		ss << COLOR_BOLD << error_title << ":" << COLOR_RESET << "\n";
		ss << "[debug] --> [line " << l_line << ", col " << l_col
		   << "] ... [line "
		   << r_line << ", col " << r_col << "]\n";
		ss << header << "\n";
		ss << prefix << source_line << "\n";

		// インデントの空白とタブを計算
		ss << std::string(line_str.length() + 4, ' ');
		for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
			if (source_line[i] == '\t')
				ss << '\t';
			else
				ss << ' ';
		}

		// 波線（Squiggles）を引く
		int length = std::max(1, static_cast<int>(end - start));
		ss << COLOR_RED;
		for (int i = 0; i < length; ++i) {
			ss << "^";
		}
		ss << COLOR_RESET << "\n";
	}
	return ss.str();
}

/*
std::string SyntaxError::to_string(const dara::core::Source& src) const {
    auto [l_line, l_col] = src.get_line_col(this->span.start);
    auto [r_line, r_col] = src.get_line_col(this->span.end);
    uint32_t file_id = src.id;
    // std::string filename = src.name;
    std::stringstream ss;

    ss << "[debug] --> [line " << l_line << ", col " << l_col << "] ... [line "
       << r_line << ", col " << r_col << "]\n";

    std::string filename = src.name;

    //  モダン化：絶対パスを「相対パス」に変換して短くする！
    try {
        std::filesystem::path p(filename);
        if (p.is_absolute()) {
            // 現在の実行ディレクトリ(カレントディレクトリ)からの相対パスを作る
            filename =
                std::filesystem::relative(p, std::filesystem::current_path())
                    .string();
        }
    } catch (const std::exception&) {
        //
万が一パス計算に失敗した場合は、元のフルパスをそのまま使う（安全第一）
    }

    std::string line_str = std::to_string(l_line);
    std::string header = std::format("{}:{}:{}: error: {}", filename, l_line,
                                     l_col, this->message);
    std::string prefix = std::format(" {} | ", line_str);
    std::string source_line = src.get_line_string(l_line);
    if (!source_line.empty()) {
        ss << "Syntax Error:\n";
        ss << header << "\n";
        ss << prefix << source_line << "\n";

        // indent prefix
        ss << std::string(line_str.length() + 4, ' ');
        for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
            if (source_line[i] == '\t') {
                ss << '\t';
            } else {
                ss << ' ';
            }
        }
        // ss << std::string(l_line, ' ');

        // 2. 波線（Squiggles）を 赤色 で引く！
        int length =
            std::max(1, static_cast<int>(this->span.end - this->span.start));
        ss << COLOR_RED;  // 波線の色を赤にする
        for (int i = 0; i < length; ++i) {
            ss << "^";
        }

        ss << COLOR_RESET << "\n";  // 色を戻す
    }

    return ss.str();
};
*/

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

/*
std::string InterpreterError::to_string(const dara::core::Source& src) const {
    // auto [line, col] = src.get_line_col(this->span.start);
    auto [l_line, l_col] = src.get_line_col(this->span.start);
    auto [r_line, r_col] = src.get_line_col(this->span.end);
    uint32_t file_id = src.id;
    //std::string filename = src.name;
    std::stringstream ss;

    std::string filename = src.name;

    //  モダン化：絶対パスを「相対パス」に変換して短くする！
    try {
        std::filesystem::path p(filename);
        if (p.is_absolute()) {
            // 現在の実行ディレクトリ(カレントディレクトリ)からの相対パスを作る
            filename =
                std::filesystem::relative(p, std::filesystem::current_path())
                    .string();
        }
    } catch (const std::exception&) {
        //
万が一パス計算に失敗した場合は、元のフルパスをそのまま使う（安全第一）
    }

    std::string line_str = std::to_string(l_line);
    ss << "[debug] --> [line " << l_line << ", col " << l_col << "] ... [line "
       << r_line << ", col " << r_col << "]\n";
    std::string header = std::format("{}:{}:{}: error: {}", filename, l_line,
                                     l_col, this->message);
    std::string prefix = std::format(" {} | ", line_str);
    // std::string indent_prefix = std::string(line_str.length() + 3, ' ');
    std::string source_line = src.get_line_string(l_line);

    if (!source_line.empty()) {
        ss << "Interpreter Error:\n";
        ss << header << "\n";
        ss << prefix << source_line << "\n";

        // indent prefix
        ss << std::string(line_str.length() + 4, ' ');
        for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
            if (source_line[i] == '\t') {
                ss << '\t';
            } else {
                ss << ' ';
            }
        }
        // ss << std::string(l_line, ' ');

        // 2. 波線（Squiggles）を 赤色 で引く！
        int length =
            std::max(1, static_cast<int>(this->span.end - this->span.start));
        ss << COLOR_RED;  // 波線の色を赤にする
        for (int i = 0; i < length; ++i) {
            ss << "^";
        }

        ss << COLOR_RESET << "\n";  // 色を戻す
    }

    return ss.str();
};
*/

std::string SyntaxError::to_string(const dara::core::Source& src) const {
	return build_error_message(src, this->span.start, this->span.end,
	                           "Fatal Error", this->message);
};

std::string InterpreterError::to_string(const dara::core::Source& src) const {
	return build_error_message(src, this->span.start, this->span.end,
	                           "Interpreter Error", this->message);
};

std::string FatalSyntaxError::to_string(const dara::core::Source& src) const {
	return build_error_message(src, this->span.start, this->span.end,
	                           "Fatal Error", this->what());
};

/*
std::string FatalSyntaxError::to_string(const dara::core::Source& src) const {
    auto [line, col] = src.get_line_col(this->span.start);

    auto [l_line, l_col] = src.get_line_col(this->span.start);
    auto [r_line, r_col] = src.get_line_col(this->span.end);
    uint32_t file_id = src.id;
    //std::string filename = src.name;
    std::stringstream ss;

    std::string filename = src.name;
    try {
        std::filesystem::path p(filename);
        if (p.is_absolute()) {
            // 現在の実行ディレクトリ(カレントディレクトリ)からの相対パスを作る
            filename =
                std::filesystem::relative(p, std::filesystem::current_path())
                    .string();
        }
    } catch (const std::exception&) {
        //
万が一パス計算に失敗した場合は、元のフルパスをそのまま使う（安全第一）
    }





    std::string line_str = std::to_string(l_line);
    ss << "[debug] --> [line " << l_line << ", col " << l_col << "] ... [line "
       << r_line << ", col " << r_col << "]\n";
    std::string header = std::format("{}:{}:{}: error: {}", filename, l_line,
                                     // l_col, this->message);
                                     l_col, what());

    std::string prefix = std::format(" {} | ", line_str);
    // std::string indent_prefix = std::string(line_str.length() + 3, ' ');
    std::string source_line = src.get_line_string(l_line);

    if (!source_line.empty()) {
        ss << "Fatal Error:\n";
        ss << header << "\n";
        ss << prefix << source_line << "\n";

        // indent prefix
        ss << std::string(line_str.length() + 4, ' ');
        for (int i = 0; i < l_col - 1 && i < source_line.length(); ++i) {
            if (source_line[i] == '\t') {
                ss << '\t';
            } else {
                ss << ' ';
            }
        }
        // ss << std::string(l_line, ' ');

        // 2. 波線（Squiggles）を 赤色 で引く！
        int length =
            std::max(1, static_cast<int>(this->span.end - this->span.start));
        ss << COLOR_RED;  // 波線の色を赤にする
        for (int i = 0; i < length; ++i) {
            ss << "^";
        }

        ss << COLOR_RESET << "\n";  // 色を戻す
    }

    return ss.str();
    //
    //std::ostringstream oss;
    //oss << "FATAL: [line " << line << ", col " << col << "] " << what();

    //// 値が存在しない（std::nullopt）なら EOF 扱い
    //if (!actual.has_value()) {
    //	oss << " (at EOF)";
    //} else {
    //	oss << ": '" << *actual << "'";
    //}
    ////	return oss.str();
    //return ss.str();
};
*/

}  // namespace dara::error
