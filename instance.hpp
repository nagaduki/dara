#pragma once
#include "class.hpp"
#include "ast.hpp"
#include "instance.hpp"
#include <memory>

namespace dara::runtime {
class Instance : public std::enable_shared_from_this<Instance> {
private:
    std::shared_ptr<Class> cls;
    std::unordered_map<std::string, Value> fields;
public:
    explicit Instance(std::shared_ptr<Class> cls) 
        : cls(std::move(cls)) {}
    Result<dara::Value> get(const std::string& name);
    void set(const std::string& name, Value value);
    std::vector<std::string> get_property_names() const;
    std::shared_ptr<Class> get_class() const;
};

} // namespace dara::backend
