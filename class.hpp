/* class.hpp */
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.hpp"       //  Value の完全な定義が必要
#include "callable.hpp"  //  Value の完全な定義が必要
#include "value.hpp"     //  Value の完全な定義が必要

namespace dara::runtime {

class Instance;  // 前方宣言（ポインタや shared_ptr の宣言だけならこれでOK）

class Class : public std::enable_shared_from_this<Class>,
              public dara::backend::Callable {
   public:
	std::string name;
	std::shared_ptr<Class> super;
	std::vector<std::shared_ptr<Class>> mixins;
	std::unordered_map<std::string, Value> methods;
	std::unordered_map<std::string, Value> static_methods;


	Class(std::string name, std::shared_ptr<Class> super,
	      std::vector<std::shared_ptr<Class>> mixins,
	      std::unordered_map<std::string, Value> methods,
	      std::unordered_map<std::string, Value> static_methods = {})
	    : name(std::move(name)),
          super(std::move(super)),
          mixins(std::move(mixins)),
	      methods(std::move(methods)),
	      static_methods(std::move(static_methods)) {}

	size_t arity() const override;
	Result<dara::Value> call(dara::backend::Interpreter& interpreter,
	                        const std::vector<dara::Value>& arguments) override;
	std::string to_string() const override;

	/*
	Class(std::string name, std::unordered_map<std::string, Value> methods)
	    : name(std::move(name)), methods(std::move(methods)) {}
	*/

	std::shared_ptr<Instance> instantiate();
	Result<dara::Value> get(const std::string& name);
	//std::optional<Value> find_method(const std::string& name);
    dara::Value find_method(const std::string& name) const;
};

}  // namespace dara::runtime
