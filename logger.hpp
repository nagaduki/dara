#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <cstdlib>

class TraceGuard {
    static inline int depth = 0;
    std::string name;
public:
    //static inline bool enabled = false;
    static inline bool enabled = (std::getenv("LOX_TRACE") != nullptr);

    TraceGuard ( std::string n ) : name (n) {
        if(!enabled) return;
        print_indent();
        std::cout << "-> Enter " << name << std::endl;
        depth++;
    }
    ~TraceGuard () {
        if(!enabled) return;
        depth--;
        print_indent();
        std::cout << "<- Enter " << name << std::endl;
    }
    void print_indent() {
        for ( int i = 0; i < depth; i++ ) {
            std::cout << "  ";
        }
    }
};
