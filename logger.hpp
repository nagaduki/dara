#pragma once

#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace dara::log {

class TraceGuard {
	static inline int depth = 0;
	std::string name;

   public:
	// static inline bool enabled = false;
	static inline bool enabled = (std::getenv("LOX_TRACE") != nullptr);

	TraceGuard(std::string n) : name(n) {
		if (!enabled) return;
		print_indent();
		std::cout << "-> Enter " << name << std::endl;
		depth++;
	}
	~TraceGuard() {
		if (!enabled) return;
		depth--;
		print_indent();
		std::cout << "<- Enter " << name << std::endl;
	}
	void print_indent() {
		for (int i = 0; i < depth; i++) {
			std::cout << "  ";
		}
	}
};

enum class Level {
	Trace = 0,
	Debug = 1,
	Info = 2,
	Warn = 3,
	Error = 4,
	Off = 5
};

inline Level current_level = Level::Info;
inline int indent_depth = 0;

template <typename... Args>
void write(Level lvl, std::string_view color, std::string_view tag,
           std::format_string<Args...> fmt, Args&&... args) {
	if (lvl < current_level) {
		return;
	}

    std::string indent(indent_depth * 2, ' ');
    std::cerr << color << "[" << tag << "] " << "\033[0m"
              << indent << std::format(fmt, std::forward<Args>(args)...) << "\n";
}

struct ScopeTrace {
    std::string_view name;
    explicit ScopeTrace(std::string_view n) : name(n) {
        write(Level::Trace, "\033[36m", "TRACE", "--> enter {}", name);
        ++indent_depth;
    }
    ~ScopeTrace() {
        --indent_depth;
        write(Level::Trace, "\033[36m", "TRACE", "<-- leave {}", name);
    }
};

#define LOG_TRACE(...) dara::log::write(dara::log::Level::Trace, "\033[36m", "TRACE", "__VA_ARGS__]");
#define LOG_DEBUG(...) dara::log::write(dara::log::Level::Trace, "\033[34m", "TRACE", "__VA_ARGS__]");
#define LOG_INFO(...) dara::log::write(dara::log::Level::Trace, "\033[32m", "TRACE", "__VA_ARGS__]");
#define LOG_WARN(...) dara::log::write(dara::log::Level::Trace, "\033[33m", "TRACE", "__VA_ARGS__]");
#define LOG_ERROR(...) dara::log::write(dara::log::Level::Trace, "\033[31m", "TRACE", "__VA_ARGS__]");


}  // namespace dara::log
