#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "value.hpp"  //  Value の完全な定義が必要
#include "ast.hpp"  //  Value の完全な定義が必要

namespace lox::runtime {

class Instance;  // 前方宣言（ポインタや shared_ptr の宣言だけならこれでOK）

class Class : public std::enable_shared_from_this<Class> {
   public:
	std::string name;
	std::unordered_map<std::string, Value> methods;

	std::shared_ptr<Class> super;
	std::vector<std::shared_ptr<Class>> mixins;

	Class(std::string name, std::unordered_map<std::string, Value> methods)
	    : name(std::move(name)), methods(std::move(methods)) {}

	std::shared_ptr<Instance> instantiate();
	Result<lox::Value> get(const std::string& name);
};

}  // namespace lox::runtime
