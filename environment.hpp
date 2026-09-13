/* environment.hpp */

#pragma once
#include <iostream>
// #include <map>
#include <optional>
#include <unordered_map>
#include <memory_resource>
// #include "interpreter.hpp"
// #include "environment.hpp"
#include "ast.hpp"
#include "value.hpp"

class Environment {
   private:
	//std::unordered_map<std::string, dara::backend::Value> variables;
	std::pmr::unordered_map<std::string, dara::backend::Value> variables;
	std::shared_ptr<Environment> enclosing;

   public:
	//Environment() : enclosing(nullptr) {};
	explicit Environment(dara::backend::Allocator alloc, std::shared_ptr<Environment> enclosing = nullptr) //(***)
	    : variables(alloc), enclosing(std::move(enclosing)) {}
	void define(const std::string& name, const dara::backend::Value& value) {
		this->variables[name] = value;
	};
	std::optional<dara::backend::Value> get(const std::string& name) {
		if (this->variables.contains(name)) {
			return variables[name];
		}

		if (enclosing != nullptr) {
			return enclosing->get(name);
		}

		return std::nullopt;
	}

	bool assign(const std::string& name, const dara::backend::Value& value) {
		if (this->variables.contains(name)) {
			this->variables[name] = value;

			return true;
		}

		if (enclosing != nullptr) {
			return enclosing->assign(name, value);
		}

		return false;
	}

    /*
	std::unordered_map<std::string, dara::backend::Value> get_all_values() {
        std::unordered_map<std::string, dara::backend::Value> variables;
		for (const auto& [name, val] : this->variables) {
			if (this->variables.contains(name)) {
                variables[name]=val;
			}
		}
		return variables;
	}
    */
	//std::unordered_map<std::string, dara::backend::Value> get_all_values() const {
	std::pmr::unordered_map<std::string, dara::backend::Value> get_all_values() const {
        return this->variables;
    }


	void dump() const {
		std::cout << "=== Environment Dump ===" << std::endl;
		for (const auto& [name, val] : variables) {
			std::cout << " " << name << " : ";
			std::cout << std::endl;
		}
		std::cout << "========================" << std::endl;
	}

	void clear() { variables.clear(); }
};
