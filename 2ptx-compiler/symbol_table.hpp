#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <unordered_map>
#include <string>
#include <memory>

struct Symbol {
    // 
    std::string name;
    std::string type;
    int scope_level;

    Symbol(const std::string& n, const std::string& k, const std::string& t, int level) : 
    name(n), type(t), scope_level(level) {}
};

class SymbolTable {
private: 
    struct Scope {
        std::unordered_map<std::string, Symbol> symbol_table;
        std::shared_ptr<Scope> parent; 
        int level;
    };

    std::shared_ptr<Scope> current_scope; 
    int next_scope_level; 
public: 
    SymbolTable() : next_scope_level(0) { // Init scope
        enter_scope(); 
    }

    void enter_scope() {
        auto new_scope = std::make_shared<Scope>(); 
        new_scope->parent = current_scope; 
        new_scope->level = ++next_scope_level; 
        current_scope = new_scope; 
    }

    void exit_scope() {
        if (current_scope->parent) {
            current_scope = current_scope->parent; 
        }
        --next_scope_level; 
    }

    Symbol* lookup(const std::string& name) {
        auto scope = current_scope; 
        while (scope) {
            auto it = scope->symbol_table.find(name); 
            if (it != scope->symbol_table.end()) {
                return &it->second; 
            }
            scope = current_scope->parent;
        }
        return nullptr; 
    }
};

#endif
