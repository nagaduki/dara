// value.cpp
// #pragma once
#include <memory>
#include <memory_resource>
#include <sstream>
#include <string>
#include <variant>

#include "callable.hpp"
// #include "lexer.hpp"
#include "value.hpp"

#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// using EvalValue = std::variant<char, int, std::string>;
// using allocator_type = std::pmr::polymorphic_allocator<Value>;

// std::string clone(std::vector<Value>::allocator_type alloc) const {
//     return "";
// }

namespace dara::backend {

// using allocator_type = std::pmr::polymorphic_allocator<Value>;

std::string to_string(const Value& val) {
	return std::visit(
	    overloaded{
	        [](int v) { return std::to_string(v); },
	        [](double v) {
		        std::ostringstream oss;
		        oss << v;
		        return oss.str();
		        // return std::to_string(v);
	        },
	        //[](const std::string& v) { return v; },
	        [](const String& v) -> std::string {
		        return "\"" + std::string(v.c_str()) + "\"";
	        },
	        [](char v) { return std::string(1, v); },
	        [](bool v) {
		        if (v) {
			        return std::string("true");
		        } else {
			        return std::string("false");
		        }
	        },
	        [](std::monostate) { return std::string("nil"); },
	        /*
	       [](const std::vector<Value>& arr) -> std::string {
	           std::string s = "[";
	           for (size_t i = 0; i < arr.size(); ++i) {
	               s += dara::backend::to_string(arr[i]);  //(*)
	               if (i != arr.size() - 1) {
	                   s += ", ";
	               }
	           }
	           s += "]";
	           return s;
	       },
	        */
	        [](const Array& arr) -> std::string {
		        std::string res = "[";
		        for (size_t i = 0; i < arr.size(); ++i) {
			        res += dara::backend::to_string(arr[i]);  //(*)
			        if (i != arr.size() - 1) {
				        res += ", ";
			        }
		        }
		        res += "]";
		        return res;
	        },
	        [](const std::shared_ptr<dara::backend::Array>& arr) -> std::string {
		        std::string res = "[";
		        // std::vector<Value> vec =
		        // std::get<std::vector<Value>>(arr);

		        // arr->at() is Value
		        // arr is vector
		        for (size_t i = 0; i < arr->size(); ++i) {
			        res += dara::backend::to_string(arr->at(i));

			        if (i != arr->size() - 1) {
				        res += ", ";
			        }
		        }
		        res += "]";
		        return res;
	        },

	        // using Map = std::pmr::unordered_map<std::string, Value>;
	        [](const std::shared_ptr<dara::backend::Map>& map) -> std::string {
		        if (!map) {
			        return "nil";
		        }

		        std::string res = "{";
		        bool first = true;

		        for (const auto& [key, value] : *map) {
			        if (!first) {
				        res += ", ";
			        }
			        res += "\"" + key + "\": ";
			        res += dara::backend::to_string(value);
			            //"\"" + key + "\": " + std::to_string(value);
			        first = false;
		        }
		        res += "}";
		        return res;
	        },

	        [](std::shared_ptr<dara::backend::Callable> c) {
		        return c->to_string();
	        },
	        [](const dara::backend::Range& r) {
		        return std::to_string(r.start) + ".." + std::to_string(r.end);
	        },

	        [](const auto&) { return std::string("unknown"); }},
	    val.data);
}

Value Value::clone(allocator_type alloc) const {
	if (auto* arr_ptr = std::get_if<std::shared_ptr<Array>>(&this->data)) {
		// auto cloned_array = std::allocate_shared<Array>(alloc, alloc);
		auto cloned_array = std::allocate_shared<Array>(alloc);

		for (const auto& elem : **arr_ptr) {
			cloned_array->push_back(elem.clone(alloc));
		}
		return Value{.data = cloned_array};
	}
	if(auto* arr_map = std::get_if<std::shared_ptr<Map>>(&this->data)) {
	//
		auto cloned_map = std::allocate_shared<Map>(alloc);

		//for (const auto& elem : **arr_map) {
		for (const auto& [key, value] : **arr_map) {
			//cloned_map->push_back(elem.clone(alloc));
            String cloned_key(key.c_str(), alloc);            
            //Value self_val{.data = this->value};

        	//return value.clone(alloc);
			//cloned_map[std::move(cloned_key)] = std::move(value.clone(alloc));
            Value cloned_val = value.clone(alloc);
			(*cloned_map)[std::move(cloned_key)] = std::move(cloned_val);


            

		}
		return Value{.data = cloned_map};

	}
	return *this;
}

}  // namespace dara::backend
