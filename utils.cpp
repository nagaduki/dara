/* utils.cpp */
//#pragma once

#include "utils.hpp"

#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <cstdio>

std::expected<void, std::string> save_to_file(
    const std::filesystem::path& filepath, const std::string& content) {
	std::ofstream file(filepath);
	if (!file) {
		return std::unexpected(
		    std::format("failed to write to: {}", filepath.string()));
	}
	std::println(file, "{}", content);
	return {};
}

bool convert_dot_to_png(const std::string& dot_ast_string,
                        const std::string& output_filepath) {
	std::string command =
	    std::format("dot -Tpng -Gdpi=57.6 -o {} 2> /dev/null", output_filepath);
	    //std::format("dot -Tpng -Gdpi=76.8 -o {} 2> /dev/null", output_filepath);
	    //std::format("dot -Tpng -O {} 2> /dev/null", output_filepath);

    FILE* pipe = popen(command.c_str(), "w");
	if (!pipe) {
		return false;
	}

    fputs(dot_ast_string.c_str(), pipe);

    int exit_status = pclose(pipe);

    return exit_status == 0;
}

#if 0
// #include <boost/process.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/pipe.hpp>
#include <boost/process/v1/search_path.hpp>

namespace bp = boost::process::v1;

bool convert_dot_to_png(const std::string& dot_ast_string,
                        const std::string& output_filepath) {
	try {
		bp::opstream pipe_into_dot;
		bp::ipstream error_from_dot;
		/*
		bp::child dot_process(
		    bp::search_path("dot"), "-Tpng", "-o", output_filepath,
		    bp::std_in<pipe_into_dot, bp::std_err> error_from_dot);
*/

		bp::child dot_process(bp::search_path("dot"), "-Tpng", "-o",
		                      output_filepath,
		                      bp::std_in<pipe_into_dot, bp::std_out> bp::null,
		                      bp::std_err > bp::null);

		pipe_into_dot << dot_ast_string << std::flush;
		pipe_into_dot.pipe().close();

		/*
		std::string line;
		while (std::getline(error_from_dot, line)) {
		    std::cerr << "Graphviz(dot)の警告/エラー: " << line << std::endl;
		}
		*/

		dot_process.wait();

		/*
		if (dot_process.exit_code() != 0) {
		    std::cerr << "画像生成失敗。終了コード: " << dot_process.exit_code()
		              << std::endl;
		    return false;
		}
		*/

		return dot_process.exit_code() == 0;
	} catch (const std::exception& e) {
		// std::cerr << "Graphviz Error: " << e.what() << std::endl;
		return false;
	}
}
#endif
