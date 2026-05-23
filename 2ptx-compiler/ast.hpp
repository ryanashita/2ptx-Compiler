#ifndef AST_HPP
#define AST_HPP

#include <iostream>
#include <cstdio>
#include <vector>
#include <unordered_map>

#include "tac_nodes.hpp"

class Expression {
public: 
    static inline std::unordered_map<std::string, std::unique_ptr<Expression>> env; // use inline keyword, applied to static members. allows initialization directly in header file

    // static inline std::vector<std::string> tac_lines; // three address code strings result
    static inline std::vector<std::unique_ptr<TACNode>> tac_nodes; 
    static inline std::unordered_map<std::string, int> expr_to_temp; // stores assigned values to remove redundancy
    static inline int temp_counter = 0; // # of temp variables in TAC, keeps track and also makes it easy to name
    int temp_id = -1; 
    static inline std::unordered_map<std::string, int> var_version; 

    virtual ~Expression() = default; 

    virtual std::unique_ptr<Expression> eval() const = 0; // no impl, all derived classes override and impl

    virtual std::string print_expr(int indent = 0) const = 0; 

    virtual std::string print_line() const = 0; 

    virtual void three_address_code() {}; 
    
    virtual std::string get_expression_key() const {return "";}; 

    virtual bool isInteger() const {
        return false; 
    } 

    static int new_temp() {
        return ++temp_counter; 
        // TODO: may need to add debugging and tests
    }

    static std::string get_var_version(const std::string& var) {
        int version = ++var_version[var]; 
        return var + std::to_string(version); 
    }

    static void print_three_address_code() {
        std::cout << "\n=== GENERATE THREE ADDRESS CODE ===\n" << std::endl; 
        for (const auto& tac_node : tac_nodes) {
            std::cout << tac_node->to_string() << "\n"; 
        }
    }
}; 

class Integer : public Expression {
public: 
    int value; 

    Integer(int v) : value{v} {}; 

    std::unique_ptr<Expression> eval() const override {
        return std::make_unique<Integer>(value); 
    }

    bool isInteger() const override {
        return true; 
    }

    std::string get_expression_key() const override {
        return std::to_string(value); 
    }

    void three_address_code() override {
        auto it = expr_to_temp.find(get_expression_key()); 
        if (it != expr_to_temp.end()) { // if the key is already in the TAC map, reuse temp_id. 
            temp_id = it->second; 
            return;
        }
        temp_id = Expression::new_temp(); // create a new temp, i.e increment temp_counter

        // std::string tac = "t" + std::to_string(temp_id) + " <- " + std::to_string(value); 
        // Expression::tac_lines.push_back(tac);

        auto tac_constant = std::make_unique<TACConstant>(Temp{temp_id}, Constant{value}); 
        Expression::tac_nodes.push_back(std::move(tac_constant)); 

        Expression::expr_to_temp[get_expression_key()] = temp_id; 
    }

    std::string print_expr(int indent = 0) const override {
        // std::cout << "print int" << std::endl; 
        return std::string(indent * 2,' ') + "Integer(" + std::to_string(value) + ")"; 
    }

    std::string print_line() const override {
        return std::to_string(value); 
    }
};

class ArithmeticExpression : public Expression {
public: 
    std::unique_ptr<Expression> left; 
    std::unique_ptr<Expression> right; 
    char op; 

    ArithmeticExpression(
        std::unique_ptr<Expression> l, 
        std::unique_ptr<Expression> r,
        char o) : left{std::move(l)}, right{std::move(r)}, op{o} {}; 
    
    std::unique_ptr<Expression> eval() const override {
        //dynamic type check
        auto lval = left->eval();
        auto rval = right->eval(); 

        //must typecheck that it is Integer
        Integer* lval_int = dynamic_cast<Integer*>(lval.get()); //dynamic_cast returns nullptr on failure. Failure means that the base class is not a derived class
        Integer* rval_int = dynamic_cast<Integer*>(rval.get());
        //The raw pointer becomes dangling after the unique_ptr deletes the memory
        if (lval_int && rval_int) {
            //after typecheck
            int ilval = lval_int->value;
            int irval = rval_int->value; 
            switch(op) {
                case '+': return std::make_unique<Integer>(ilval + irval); 
                case '-': return std::make_unique<Integer>(ilval - irval);
                case '*': return std::make_unique<Integer>(ilval * irval); 
                case '/': 
                    if (irval == 0) {
                        throw std::runtime_error("Division by zero");
                    } 
                    return std::make_unique<Integer>(ilval / irval);
                default: throw std::runtime_error("Unknown operator"); 
            }
        } else {
            throw std::runtime_error("Cannot add nodes that are not Integer"); 
        }
    }

    std::string get_expression_key() const override {
        std::string left_key = left->get_expression_key(); 
        std::string right_key = right->get_expression_key(); 
        return "(" + left_key + " " + op + " " + right_key + ")";
    }

    void three_address_code() override {
        left->three_address_code(); // run TAC on left and right sides, otherwise can't generate most optimal TAC lines for this node
        right->three_address_code();
        
        std::string key = get_expression_key(); 

        auto it = Expression::expr_to_temp.find(key); 
        if (it != Expression::expr_to_temp.end()) {
            temp_id = it->second; 
            return;
        }

        temp_id = Expression::new_temp(); 

        // std::string tac = "t" + std::to_string(temp_id) + " <- " + 
        //     "t" + std::to_string(left->temp_id) + " " + 
        //     op + " " + 
        //     "t" + std::to_string(right->temp_id); 
        // Expression::tac_lines.push_back(tac);

        TACOp ope;
        switch(op) {
            case '+': ope = TACOp::ADD; break; 
            case '-': ope = TACOp::SUB; break; 
            case '*': ope = TACOp::MUL; break; 
            case '/': ope = TACOp::DIV; break; 
        }
        auto tac_binary = std::make_unique<TACBinaryOp>(Temp{temp_id}, Temp{left->temp_id}, Temp{right->temp_id}, ope);
        Expression::tac_nodes.push_back(std::move(tac_binary)); 

        Expression::expr_to_temp[key] = temp_id; 
    }

    std::string print_expr(int indent = 0) const override {
        return std::string(indent * 2,' ') + "Arithmetic(\n" + left->print_expr(indent + 1) + ",\n" + std::string(indent * 2 + 2,' ') + op + ",\n" + right->print_expr(indent + 1) + ")"; 
    }

    std::string print_line() const override {
        return "(" + left->print_line() + " " + op + " " + right->print_line() + ")"; 
    }
}; 

class Variable : public Expression {
public:
    std::string varname; 

    Variable(std::string n) : varname{n} {}; 

    std::unique_ptr<Expression> eval() const override {
        auto expr = Expression::env.find(varname); 
        if (expr == env.end()) {
            throw std::runtime_error("undefined variable: " + varname);
        }
        return expr->second->eval(); 
    }

    std::string get_expression_key() const override {
        return varname + std::to_string(Expression::var_version[varname]); 
    }

    void three_address_code() override {
        std::string key = get_expression_key(); 
        auto it = Expression::expr_to_temp.find(key); 
        if (it != Expression::expr_to_temp.end()) { // same logic as integer
            temp_id = it->second; 
            return; 
        }
        temp_id = new_temp(); 

        // TODO check the logic on this 
        if (Expression::var_version.find(varname) == var_version.end()) {
            return;
        }

        // std::string tac = "t" + std::to_string(temp_id) + " <- "  + varname + std::to_string(Expression::var_version[varname]); 
        // Expression::tac_lines.push_back(tac);

        auto tac_variable = std::make_unique<TACLoad>(TACVariable{varname + std::to_string(Expression::var_version[varname])}, Temp{temp_id});
        Expression::tac_nodes.push_back(std::move(tac_variable)); 

        Expression::expr_to_temp[key] = temp_id; 
    }

    std::string print_expr(int indent = 0) const override {
        return std::string(indent * 2,' ') + "Variable(" + varname + ")"; 
    }

    std::string print_line() const override {
        return varname; 
    }
};

class AssignmentExpression : public Expression {
public:
    std::string var; 
    std::unique_ptr<Expression> expr; 

    AssignmentExpression(
        std::string v, 
        std::unique_ptr<Expression> e) : var{std::move(v)}, expr{std::move(e)} {};
    
    std::unique_ptr<Expression> eval() const override {
        auto evaluated = expr->eval();
        Expression::env[var] = std::move(evaluated); 
        return Expression::env[var]->eval(); //TODO: probably keep but note
    }

    std::string get_expression_key() const override {
        return ""; 
    }

    void three_address_code() override {
        // assignments modify state
        expr->three_address_code(); 

        std::string key = expr->get_expression_key(); 

        auto it = Expression::expr_to_temp.find(key); 
        if (it == Expression::expr_to_temp.end()) { // if no key for the rvalue, something went wrong. return 
            std::exit(1);  
        }
        temp_id = it->second; 
        // don't return here, need to save the temp_id and use it as rvalue below
        std::string new_ssa_name = Expression::get_var_version(var);
        // std::string tac1 = new_ssa_name + " <- " + "t" + std::to_string(temp_id); 
        // Expression::tac_lines.push_back(tac1);

        auto tac_assignment = std::make_unique<TACStore>(TACVariable{new_ssa_name}, Temp{temp_id}); 
        Expression::tac_nodes.push_back(std::move(tac_assignment)); 

        temp_id = expr->temp_id; 
    } 

    std::string print_expr(int indent = 0) const override {
        return std::string(indent * 2,' ') + "Assignment(\n" + std::string(indent * 2 + 2,' ') + '\'' + var + '\'' + ",\n" + expr->print_expr(indent + 1) + ")"; 
    }

    std::string print_line() const override {
        return var + " = " + expr->print_line(); 
    }
};

class ExpressionList : public Expression {
public: 
    std::vector<std::unique_ptr<Expression>> expression_list; 
    
    // default constructor, add to the vector with the add method
    ExpressionList() = default; 

    std::unique_ptr<Expression> eval() const override {
        auto result_list = std::make_unique<ExpressionList>();
        for (const auto& expr : expression_list) {
            auto evaluated = expr->eval(); 
            result_list->add(std::move(evaluated)); 
        }
        
        return std::move(result_list->expression_list[expression_list.size() - 1]); 
    }

    void add(std::unique_ptr<Expression> expr) {
        expression_list.push_back(std::move(expr)); 
    }

    void three_address_code() override {
        for (const auto& expr : expression_list) {
            expr->three_address_code(); 
        }
    }

    std::string print_expr(int indent = 0) const override {
        // std::cout << "print exprlist" << std::endl; 
        std::string result = std::string(indent * 2,' ') + "ExprList[\n"; 
        for (int i = 0; i < expression_list.size(); ++i) {
            result += expression_list.at(i)->print_expr(indent + 1); 
            if (i + 1 < expression_list.size()) {
                result += ",\n"; // add comma between expressions
            }
        }
        return result += "]"; 
    }

    std::string print_line() const override {
        std::string result = "["; 
        for (size_t i = 0; i < expression_list.size(); ++i) {
            result += expression_list.at(i)->print_line(); 
            if (i + 1 < expression_list.size()) {
                result += ","; // add comma between expressions
            }
        }
        return result += "]"; 
    }
};

class Vector : public Expression { // math definition of a vector
public: 
    size_t size; 
    std::vector<std::unique_ptr<Integer>> int_vector; 

    // default vector constructor. values will be added to vector instance with add function
    Vector() = default; 

    std::unique_ptr<Expression> eval() const override {
        auto result_vector = std::make_unique<Vector>(); 
        for (const auto& expr : int_vector) {
            auto evaluated = expr->eval(); 
            result_vector->add(std::move(evaluated)); 
        }
        return result_vector; 
    }

    void add(std::unique_ptr<Expression> expr) {
        // if the thing being added is a Integer, then allow the add. 
        auto int_expr = std::unique_ptr<Integer>(dynamic_cast<Integer*>(expr.get()));
        if (int_expr) {
            expr.release(); 
            int_vector.push_back(std::move(int_expr)); 
        } else {
            throw std::runtime_error("Can only add 'Integer' nodes to 'Vector' node"); 
        }
    } 

    std::string print_expr(int indent = 0) const override {
        std::string result = std::string(indent * 2, ' ') + "Vector[\n"; 
        for (int i = 0; i < int_vector.size(); ++i) {
            result += int_vector.at(i)->print_expr(indent + 1); 
            if (i + 1 < int_vector.size()) {
                result += ",\n"; // add comma between integers
            }
        }
        return result += ']'; 
    }

    std::string print_line() const override {
        std::string result = "["; 
        for (int i = 0; i < int_vector.size(); ++i) {
            result += int_vector.at(i)->print_line(); 
            if (i + 1 < int_vector.size()) {
                result += ","; // add comma between integers
            }
        }
        return result += ']'; 
    }
};

class Term : public Expression {};

class Parens : public Expression {
public: 
    char left_parens;
    std::unique_ptr<Expression> expr; 
    char right_parens; 
}; 

#endif