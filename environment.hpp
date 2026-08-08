/* environment.hpp */

#pragma once
#include <iostream>
//#include <map>
#include <optional>
#include <unordered_map>
// #include "interpreter.hpp"
// #include "environment.hpp"
#include "ast.hpp"
#include "value.hpp"

class Environment {
   private:
	std::unordered_map<std::string, lox::Value> variables;
	std::shared_ptr<Environment> enclosing;

   public:
	Environment() : enclosing(nullptr) {};
	explicit Environment(std::shared_ptr<Environment> enclosing)
	    : enclosing(std::move(enclosing)) {}
	void define(const std::string& name, const lox::Value& value) {
		variables[name] = value;
	};
	std::optional<lox::Value> get(const std::string& name) {
		if (variables.contains(name)) {
			return variables[name];
		}

		if (enclosing != nullptr) {
			return enclosing->get(name);
		}

		return std::nullopt;
	}

	bool assign(const std::string& name, const lox::Value& value) {
		if (variables.contains(name)) {
			variables[name] = value;

			return true;
		}

		if (enclosing != nullptr) {
			return enclosing->assign(name, value);
		}

		return false;
	}

	void dump() const {
		std::cout << "=== Environment Dump ===" << std::endl;
		for (const auto& [name, val] : variables) {
            std::cout << " " << name << " : ";
            std::cout << std::endl;
		}
		std::cout << "========================" << std::endl;
	}
    
	void clear() {
        variables.clear();
    }
};
