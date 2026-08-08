/* combinator.hpp */

#pragma once  // インクルードガードのモダンな書き方
#include <expected>
#include <functional>
#include <string>
#include "error.hpp"
#include "source.hpp"

template <typename T>
using Rule = std::function<std::expected<T, SyntaxError>(Source*)>;

template <typename T>
Rule<T> left(const std::string& e) {
	Rule<T> np = [=](Source* s) -> std::expected<T, SyntaxError> {
		return std::unexpected(s->make_error(e));
	};
	return np;
}

inline Rule<char> left(const std::string& e) { return left<char>(e); }

template <typename T>
Rule<T> operator||(const Rule<T>& p1, const Rule<T>& p2) {
	return [=](Source* s) -> std::expected<T, SyntaxError> {
		Source backup = *s;

		return p1(s).or_else([&](const SyntaxError& err1) {
			*s = backup;

			return p2(s).transform_error([&](const SyntaxError& err2) {
				// 【最深部エラー優先戦略】
				// err1 の方がソースコードの後ろの方で発生していたら、err1
				// を採用する
				if (err1.line > err2.line ||
				    (err1.line == err2.line && err1.col > err2.col)) {
					return err1;
				}
				// err2 の方が後ろ、または全く同じ位置で失敗した場合は、
				// 最後に試した fallback（代替案）である err2 を採用する
				else {
					return err2;
				}
			});
		});
	};
}

template <typename T>
Rule<std::string> many(const Rule<T>& p) {
	Rule<std::string> np =
	    [=](Source* s) -> std::expected<std::string, SyntaxError> {
		std::string ret;
		while (auto res = p(s)) {
			ret += res.value();
		}
		return ret;
	};
	return np;
}

template <typename T>
Rule<std::string> many1(const Rule<T>& p) {
	return p + many(p);
}

/* satify 1 */
inline Rule<char> satisfy(const std::function<bool(char)>& f) {
	Rule<char> np = [=](Source* s) -> std::expected<char, SyntaxError> {
		if (s->isEnd()) return std::unexpected(s->make_error("too short"));
		auto ch = s->peek();
		if (f(ch.value()) != true)
			return std::unexpected(s->make_error("not satisfy"));
		s->next();
		return ch;
	};
	return np;
}

/* satify 2 */
inline Rule<char> satisfy(const std::function<bool(char, char)>& f, char c) {
	// Rule<char>の型に合わせて、引数は Source* のみとする
	// 比較対象の文字 'c' と関数 'f' は [=] でキャプチャ（コピー）して内部で使う
	Rule<char> np = [=](Source* s) -> std::expected<char, SyntaxError> {
		auto ch_res = s->peek();  // error
		if (!ch_res) return std::unexpected(ch_res.error());
		if (f(ch_res.value(), c) != true)
			return std::unexpected(s->make_error("not satisfy"));
		;
		s->next();
		return ch_res;
	};
	return np;
}


// operator<< 
template <typename T1, typename T2>
Rule<T1> operator<<(const Rule<T1>& p1, const Rule<T2>& p2) {
	return [=](Source* s) -> std::expected<T1, SyntaxError> {
		Source loop_backup = *s;
		auto res_left = p1(s);
		if (!res_left) {
			*s = loop_backup;
			return std::unexpected(res_left.error());
		}
		auto res_right = p2(s);
		if (!res_right) {
			*s = loop_backup;
			return std::unexpected(res_right.error());
		}
		return res_left;
	};
}



template <typename T1, typename T2>
Rule<T2> operator>>(const Rule<T1>& p1, const Rule<T2>& p2) {
	return [=](Source* s) -> std::expected<T2, SyntaxError> {
		// return [=](Source *s) -> Rule<T2> {
		Source loop_backup = *s;
		auto res_left = p1(s);
		if (!res_left) {
			*s = loop_backup;
			return std::unexpected(res_left.error());
		}
		auto res_right = p2(s);
		if (!res_right) {
			*s = loop_backup;
			return std::unexpected(res_right.error());
		}
		return res_right;
	};
}

template <typename T1, typename T2>
Rule<std::string> operator+(const Rule<T1>& x, const Rule<T2>& y) {
	return [=](Source* s) -> std::expected<std::string, SyntaxError> {
		Source backup = *s;

		// x を実行し、成功したら(and_then)その値(val_x)を持って次へ
		return x(s).and_then([&](auto val_x) {
			// y を実行し、成功したら(transform) xとyの値を結合する
			auto res_y = y(s);
			if (!res_y) *s = backup;  // ※yが失敗した時の巻き戻しだけは必要

			return res_y.transform([&](auto val_y) {
				std::string ret;
				ret += val_x;
				ret += val_y;
				return ret;
			});
		});
	};
}

template <typename T>
Rule<std::string> operator*(int n, const Rule<T>& x) {
	// Rule<std::string> np = [=](Source* s) -> std::optional<string> {
	Rule<std::string> np =
	    [=](Source* s) -> std::expected<std::string, SyntaxError> {
		Source backup = *s;
		std::string ret = "";
		for (int i = 0; i < n; i++) {
			// ret += x(s);
			if (auto res = x(s)) {
				ret += res.value();
			} else {
				*s = backup;
				return std::unexpected(res.error());
			}
		}
		return ret;
	};
	return np;
}

template <typename T>
Rule<std::string> operator*(const Rule<T>& x, int n) {
	return n * x;
}

template <typename T1, typename T2>
Rule<T1> apply(const std::function<T1(const T2&)>& f, const Rule<T2>& p) {
	return [=](Source* s) -> std::expected<T1, SyntaxError> {
		Source loop_backup = *s;
		auto res_p = p(s);
		if (!res_p) {
			*s = loop_backup;
			return std::unexpected(res_p.error());
		}
		auto res_f = f(res_p.value());
		return res_f;
	};
}

template <typename T>
    requires requires(T x) { -x; }
Rule<T> operator-(const Rule<T>& p) {
	// return apply<T, T>([](T x){ return - x; }, p);
	return apply<T, T>(std::negate<T>(), p);
}


