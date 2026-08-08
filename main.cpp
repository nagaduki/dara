/* main.cpp */
#include <iostream>
#include <string>

#include "ast.hpp"
#include "interpreter.hpp"
#include "linenoise.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "utils.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

void run_file(const char* file_path) {
	DotPrinter dotprinter;
	std::string file_name = "main_run";
	std::ifstream file(file_path);
	if (!file.is_open()) {
		std::cerr << "Could not open file: " << file_path << std::endl;
		std::exit(74);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string source_code = buffer.str();

	Source s(source_code.c_str());
	lox::frontend::Parser p(&s);
	lox::backend::Interpreter interpreter;

	/*
	lox::Program program;
	while (!s.isEnd()) {
	    s.skip_whitespace();
	    spaces(&s);
	    if (s.isEnd()) {
	        break;
	    }

	    auto res = p.decl();
	    // auto res = parser.program();
	    if (!res.has_value()) {
	        std::cerr << "Error: " << res.error().message << "\n";
	        program.declarations.clear();
	        // continue;
	        break;
	    }
	    program.declarations.push_back(std::move(res.value()));
	}
	*/

	auto program_res = p.program();
	if (!program_res) {
		std::cerr << "syntax error: " << program_res.error().message << "\n";
		return;
	}
	lox::Program program = std::move(program_res.value());

	std::string dot_result = dotprinter.print(&program);
	auto save_res =
	    save_to_file(std::format("./{}.dot", file_name), dot_result);
	if (!save_res) {
		std::cerr << "failed to save dot file Error: " << save_res.error()
		          << "\n";
	}

	if (!convert_dot_to_png(dot_result, std::format("./{}.png", file_name))) {
		std::cerr << "failed to convert png file Error: " << "\n";
	}

	for (const auto& decl : program.declarations) {
		auto val = interpreter.exec(decl.get());
		if (!val) {
			std::cerr << "Interpreter Error: " << val.error().message << "\n";
			break;
		}
	}
}
void run_repl() {
	lox::backend::Interpreter interpreter;
	// DotPrinter printer;
	linenoise::SetHistoryMaxLen(100);
	std::string file_name = "main_repl";

	std::cout << "--------------------------\n";
	std::cout << "welcome to Dara REPL\n";
	std::cout << "(Press Ctrl-D or type 'exit' to quit)\n";
	std::cout << "--------------------------\n";

	std::string input;

	while (true) {
		DotPrinter dotprinter;
		std::string line;
		bool quit = linenoise::Readline(">> ", input);
		if (quit) {
			break;
		}
		if (!input.empty()) {
			linenoise::AddHistory(input.c_str());
		}
		if (input == "exit" || input == "quit") {
			break;
		}
		if (input.empty()) {
			continue;
		}

		try {
			Source s(input.c_str());
			lox::frontend::Parser parser(&s);
			// std::vector<std::unique_ptr<lox::Decl>> program_ast;
			lox::Program program;
			// auto res = parse_expr(&s);

			/* main loop */
			while (!s.isEnd()) {
				s.skip_whitespace();
				spaces(&s);
				if (s.isEnd()) {
					break;
				}

                /*
				auto res = parser.decl();
				// auto res = parser.program();
				if (!res.has_value()) {
					std::cerr << "Error: " << res.error().message << "\n";
					program.declarations.clear();
					// continue;
					break;
				}
				program.declarations.push_back(std::move(res.value()));
                */

				auto program_res = parser.program();
				if (!program_res) {
					std::cerr << "syntax error: " << program_res.error().message
					          << "\n";
					return;
				}
				program = std::move(program_res.value());
			}
			if (program.declarations.empty()) {
				continue;
			}

			// std::string dot_result = dotprinter.print(program.declarations);
			std::string dot_result = dotprinter.print(&program);

			auto save_res =
			    save_to_file(std::format("./{}.dot", file_name), dot_result);

			if (!save_res) {
				// MESSAGE("failed to save dot file.", save_res.error());
				std::cerr << "failed to save dot file Error: "
				          << save_res.error() << "\n";
			}

			if (!convert_dot_to_png(dot_result,
			                        std::format("./{}.png", file_name))) {
				std::cerr << "failed to convert png file Error: " << "\n";
			}

			for (const auto& decl : program.declarations) {
				auto val = interpreter.exec(decl.get());
				if (!val) {
					std::cerr << "Interpreter Error: " << val.error().message
					          << "\n";
					break;
				}
			}

			std::cout << "status : success\n";

		} catch (const std::exception& e) {
			// PRINT_LINE();
			std::cerr << "Fatal Error: " << e.what() << std::endl;
		}

		// auto val = interpreter.eval(res.value().get());
	}

	std::cout << "bye. \n";
}

int main(int argc, char* argv[]) {
	// TraceGuard::enabled = true;
	if (argc == 1) {
		run_repl();
	} else if (argc == 2) {
		run_file(argv[1]);
	} else {
		std::cerr << "Usage: cpplox [script_file]" << std::endl;
		return 64;
	}
	return 0;
}
