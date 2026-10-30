/* class.hpp */
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "callable.hpp"
#include "value.hpp"

namespace dara::backend {

class Instance;

class OverloadedConstructor {
   private:
	std::unordered_map<size_t, std::shared_ptr<Function>> constructors;

   public:
	OverloadedConstructor() = default;

	bool add(std::shared_ptr<Function> ctor) {
		size_t arity = ctor->arity();
		if (constructors.contains(arity)) {
			return false;
		}
		constructors.emplace(arity, std::move(ctor));
		return true;
	}
	bool empty() const noexcept { return constructors.empty(); }

	std::shared_ptr<Function> find(size_t arity) const {
		auto it = constructors.find(arity);
		if (it != constructors.end()) {
			return it->second;
		}
        return nullptr;
	}
};

class Class : public std::enable_shared_from_this<Class>,
              public dara::backend::Callable {
   public:
	std::string name;
	std::shared_ptr<Class> super;
	std::vector<std::shared_ptr<Class>> mixins;
	std::unordered_map<std::string, Value> methods;
	std::unordered_map<std::string, Value> static_methods;
	//

	// java like class
    OverloadedConstructor constructors;
	std::unordered_set<std::string> fields;
	std::vector<std::pair<std::string, const dara::ast::Expr*>>
	    field_initializers;

	Class(std::string name, 
          std::shared_ptr<Class> super,
	      std::vector<std::shared_ptr<Class>> mixins,
	      std::unordered_map<std::string, Value> methods,
	      std::unordered_map<std::string, Value> static_methods = {},
          OverloadedConstructor constructors = {},
	      std::unordered_set<std::string> fields = {},
	      std::vector<std::pair<std::string, const dara::ast::Expr*>>
	          field_initializers = {})
	    : name(std::move(name)),
	      super(std::move(super)),
	      mixins(std::move(mixins)),
	      methods(std::move(methods)),
	      static_methods(std::move(static_methods)),
	      constructors(std::move(constructors)),
	      fields(std::move(fields)),
	      field_initializers(std::move(field_initializers)){}

	size_t arity() const override;
	dara::ast::Result<Value> call(dara::backend::Interpreter& interpreter,
	                              const std::vector<Value>& arguments) override;
	std::string to_string() const override;

	/*
	Class(std::string name, std::unordered_map<std::string, Value> methods)
	    : name(std::move(name)), methods(std::move(methods)) {}
	*/

	std::shared_ptr<Instance> instantiate();
	dara::ast::Result<Value> get(Interpreter& interpreter,
	                             const std::string& name);
	// std::optional<Value> find_method(const std::string& name);

	//
	// java like class
	Value find_method(const std::string& method_name) const;
	bool has_field(const std::string& field_name) const;

	//dara::ast::Result<void> init_default_fields(
	dara::ast::Result<void> init_fields(
	    Interpreter& interpreter,
	    const std::shared_ptr<Instance>& instance) const;
};

}  // namespace dara::backend
