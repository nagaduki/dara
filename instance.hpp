#pragma once
#include "class.hpp"
#include "ast.hpp"
#include "instance.hpp"
#include <memory>

namespace dara::backend {
class Instance : public std::enable_shared_from_this<Instance> {
private:
    std::shared_ptr<Class> cls;
    std::unordered_map<std::string, Value> fields;
public:
    explicit Instance(std::shared_ptr<Class> cls) 
        : cls(std::move(cls)) {}
    dara::ast::Result<Value> get(Interpreter& interpreter, const std::string& name);
    void set(const std::string& name, Value value);
    std::vector<std::string> get_property_names() const;
    std::shared_ptr<Class> get_class() const;
};

} // namespace dara::backend
