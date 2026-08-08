/* utils.hpp */
#pragma once

#include <filesystem>
#include <expected>
#include <string>
//#include <iostream>
//#include <boost/process.hpp>
//#include <fstream>
//#include <format>

std::expected<void, std::string> save_to_file(const std::filesystem::path& filepath, const std::string& content);
bool convert_dot_to_png(const std::string& dot_ast_string, const std::string& output_filepath);
