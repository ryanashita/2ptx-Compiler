#ifndef PARSER_HPP
#define PARSER_HPP

#include <iostream>
#include <vector>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <type_traits>

#include "ast.hpp"

// g++ parser.cpp -std=c++17 -O2 -o parser

namespace pegtl = tao::pegtl; //namespace alias, just a nickname

namespace grammar {

	struct arith_expr; 

	struct ws : pegtl::star<pegtl::space> {};

	// impl pos/neg numbers like so, or make them two separate entities
	struct number 
		: pegtl::seq<
			ws,
			pegtl::opt<pegtl::one<'+','-'>>,
			pegtl::plus<pegtl::digit>,
			ws
		> {}; 

	struct variable : pegtl::identifier {}; 

	struct term 
		: pegtl::sor<
			pegtl::seq<
				pegtl::one<'('>,
				arith_expr,
				pegtl::one<')'>
			>,
			number,
			variable
		> {}; 

	struct arith_op : pegtl::one<'+','-','*','/'> {};
	
	struct assign_op
		: pegtl::seq<
			ws,
			pegtl::one<'='>,
			ws
		> {}; 

	struct arith_expr 
		: pegtl::seq<
			ws, 
			term,
			pegtl::star<
				pegtl::seq<
					ws,
					arith_op,
					ws,
					term
				>
			>,
			ws
		> {}; 
	
	struct vector 
		: pegtl::seq<
			pegtl::one<'['>,
			ws,
			pegtl::seq<
				number,
				pegtl::star<
					pegtl::seq<
						pegtl::one<','>,
						number
					>
				>
			>,
			ws,
			pegtl::one<']'>
		> {}; 

	struct expr 
		: pegtl::sor<
			number,
			vector,
			arith_expr,
			variable
		> {}; 
	
	struct assign_expr 
		: pegtl::seq<
			variable,
			assign_op,
			expr,
			ws
		> {}; 

	struct program 
		: pegtl::seq<
			ws,
			pegtl::list_tail<
				pegtl::sor<
					assign_expr,
					arith_expr,
					vector,
					variable
				>,
				pegtl::seq<ws,pegtl::one<';'>,ws>
			>,
			ws,
			pegtl::eof
		> {}; 
}; 

struct my_ast_node : pegtl::parse_tree::node {
	std::unique_ptr<Expression> ast; // ast holds the Expression nodes (my custom AST representation)

	// default constructors
	my_ast_node() = default;
    my_ast_node(const my_ast_node&) = delete;
    my_ast_node(my_ast_node&&) = delete;
    my_ast_node& operator=(const my_ast_node&) = delete;
    my_ast_node& operator=(my_ast_node&&) = delete;
}; 

template< typename Rule >
struct selector :
	pegtl::parse_tree::selector< 
		Rule, 
		pegtl::parse_tree::store_content::on<
			grammar::arith_op,
			/*
			Don't need to include these rules in the base selector; they have custom selectors (template specializations)
				grammar::arith_expr,
				grammar::expr,
				grammar::number,
				grammar::term,
				grammar::program,
				grammar::assign_expr,
				grammar::vector
			*/
			grammar::variable,
			grammar::assign_op
		>
> {}; 

template<>
struct selector< grammar::number> : std::true_type {
	template< typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- NUMBER TRANSFORM ---" << std::endl;

		if (!node->has_content()) {
			std::cout << "has_content: 0" << std::endl;
			return;
		} 

		std::string content = node->string();
        std::cout << "content: '" << content << "'" << std::endl; // cout for debug

		// assigning ast the Integer object with value		
		int value = std::stoi(node->string()); 
		node->ast = std::make_unique<Integer>(value); 
	}
};

template<>
struct selector< grammar::term> : std::true_type {
	template< typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- TERM TRANSFORM ---" << std::endl;

		if (node->children.empty()) {
			std::cout << "term children count: 0" << std::endl;
			return;
		} 

		std::cout << "term children count: " << node->children.size() << std::endl;
		for (size_t i = 0; i < node->children.size(); ++i) {
            auto* child = static_cast<my_ast_node*>(node->children[i].get());
            std::cout << "  child[" << i << "] type: " << child->type;
            if (child->has_content()) {
                std::cout << " content: '" << child->string() << "'";
            }
            std::cout << " has_ast: " << (child->ast ? "YES" : "NO") << std::endl;
        }

		// parsing number
		if (node->children.size() == 1) {
			auto* child = static_cast<my_ast_node*>(node->children[0].get()); 
			assert(child->ast && "Integer is null in term"); 
			node->ast = std::move(child->ast); 
		
		// parsing arith_expr
		} else if (node->children.size() == 3) {
			auto* arith_child = static_cast<my_ast_node*>(node->children[1].get()); // need the arith_expr in index 1, ignore parens
			assert(arith_child->ast && "arithexpr is null in term"); 
			node->ast = std::move(arith_child->ast); 
		} else {
			throw std::runtime_error("unexpected children count"); 
		}
		assert(node->ast && "term failed to produce AST");
	}
};

template<>
struct selector< grammar::variable > : std::true_type {
	template< typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- VARIABLE TRANSFORM ---" << std::endl;

		if (!node->has_content()) {
			std::cout << "has_content: 0" << std::endl;
			return;
		} 

		std::string content = node->string();
        std::cout << "content: '" << content << "'" << std::endl; // cout for debug

		// assigning ast the Variable object with value		
		node->ast = std::make_unique<Variable>(node->string()); 
		assert(node->ast && "variable failed to produce AST");
	}
};

template<>
struct selector< grammar::arith_expr > : std::true_type {
	template< typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- ARITH_EXPR TRANSFORM ---" << std::endl;

		if (node->children.empty()) {
			std::cout << "arith children count: 0" << std::endl;
			return;
		} 
		
		std::cout << "arith children count: " << node->children.size() << std::endl;
		for (size_t i = 0; i < node->children.size(); ++i) {
            auto* child = static_cast<my_ast_node*>(node->children[i].get());
            std::cout << "  child[" << i << "] type: " << child->type;
            if (child->has_content()) {
                std::cout << " content: '" << child->string() << "'";
            }
            std::cout << " has_ast: " << (child->ast ? "YES" : "NO") << std::endl;
        }

		auto* left_node = static_cast<my_ast_node*>(node->children[0].get()); 
		assert(left_node->ast && "Left AST is null in arith_expr"); 

		/*
		Loop through arith_expr children
			Ex: 1+1+1 -> ((1 + 1) + 1)
			left already owns the first number. iterate through next two to create next arithmetic triple

		*/
		std::unique_ptr<Expression> left = std::move(left_node->ast);
		for (size_t i = 1; i < node->children.size(); i += 2) {
			char op = node->children[i]->string()[0]; // grab the first char in the string, which is always op, char length 1 

			// grab right node, which is next child hence i+1
			auto* right_node = static_cast<my_ast_node*>(node->children[i+1].get()); 
			assert(right_node->ast && "Right AST is null in arith_expr"); 

			// std::unique_ptr<Expression> right = std::move(right_node->ast);
			// auto right = std::move(static_cast<my_ast_node*>(node->children[i+1].get())->ast);
			// if (!right) return; 
			left = std::make_unique<ArithmeticExpression>(
				std::move(left), std::move(right_node->ast), op
			); 
		}
		node->ast = std::move(left); 
		// std::cout << typeid(node->ast).name(); // how to check type, save for future use
		assert(node->ast && "arith_expr failed to produce AST");
	}
};

template<>
struct selector< grammar::assign_expr > : std::true_type {
	template<typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- ASSIGN_EXPR TRANSFORM ---" << std::endl;

		if (node->children.empty()) {
			std::cout << "assign children count: 0" << std::endl;
			return;
		} 

		// should only have three children: variable, operator, expr
		std::cout << "assign children count: " << node->children.size() << std::endl;
		for (size_t i = 0; i < node->children.size(); ++i) {
            auto* child = static_cast<my_ast_node*>(node->children[i].get());
            std::cout << "  child[" << i << "] type: " << child->type;
            if (child->has_content()) {
                std::cout << " content: '" << child->string() << "'";
            }
            std::cout << " has_ast: " << (child->ast ? "YES" : "NO") << std::endl;
        }

		auto* variable_node = static_cast<my_ast_node*>(node->children[0].get()); 
		if (!variable_node->has_content()) {
			throw std::runtime_error("variable AST is null/has no content");
		}		
		std::string variable_name = variable_node->string(); 

		auto* content_node = static_cast<my_ast_node*>(node->children[2].get()); 
		assert(content_node->ast && "content expr AST is null in assign_expr");
		
		std::unique_ptr<Expression> content = std::move(content_node->ast);
		auto assignment = std::make_unique<AssignmentExpression>(variable_name, std::move(content));
		node->ast = std::move(assignment); 
	}
};

template<>
struct selector< grammar::vector > : std::true_type {
	template<typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- VECTOR TRANSFORM ---" << std::endl; 

		if (node->children.empty()) {
			std::cout << "vector children count: 0. shouldn't be possible to parse." << std::endl;
			return;
		}
		std::cout << "vector children count: " << node->children.size() << std::endl; 
		for (size_t i = 0; i < node->children.size(); ++i) {
			auto* child = static_cast<my_ast_node*>(node->children[i].get()); 
			std::cout << "  child[" << i << "] type: " << child->type;			
			if (child->has_content()) {
				std::cout << " content: '" << child->string() << "'"; 
			}
			std::cout << " has_ast: " << (child->ast ? "YES" : "NO") << std::endl;
		}

		// initialize Vector object
		auto vector_expr = std::make_unique<Vector>(); 
		for (auto& child : node->children) {
			auto* my_ast_node_ch = static_cast<my_ast_node*>(child.get()); 
			if (!my_ast_node_ch->ast) {
				std::cout << "skipping null child ast in vector" << std::endl; 
				continue; 
			}
			vector_expr->add(std::move(my_ast_node_ch->ast)); // add from child node to vector
		}
		std::cout << "Vector size: " << vector_expr->int_vector.size() << std::endl;
		node->ast = std::move(vector_expr); 
		assert(node->ast && "vector failed to produce AST");
	}
};

template<>
struct selector< grammar::program > : std::true_type {
	template<typename... States>
	static void transform(std::unique_ptr<my_ast_node>& node, States&&...) {
		std::cout << "\n--- PROGRAM TRANSFORM ---" << std::endl;

		if (node->children.empty()) {
			std::cout << "program children count: 0" << std::endl;
			return;
		}
		std::cout << "program (elist) children count: " << node->children.size() << std::endl;
		for (size_t i = 0; i < node->children.size(); ++i) {
            auto* child = static_cast<my_ast_node*>(node->children[i].get());
            std::cout << "  child[" << i << "] type: " << child->type;
            if (child->has_content()) {
                std::cout << " content: '" << child->string() << "'";
            }
            std::cout << " has_ast: " << (child->ast ? "YES" : "NO") << std::endl;
        }

		// initialize ExpressionList object + unique_ptr
		auto elist = std::make_unique<ExpressionList>(); 
		for (auto& child : node->children) {
			// c++ ownership semantics
			auto* my_ast_node_ch = static_cast<my_ast_node*>(child.get()); 
			if (!my_ast_node_ch->ast) {
				std::cout << "skipping null child ast in program" << std::endl; 
				continue; 
			}
			elist->add(std::move(my_ast_node_ch->ast)); // add from child node to elist
		}
		std::cout << "ExpressionList size: " << elist->expression_list.size() << std::endl;
		node->ast = std::move(elist); 
		assert(node->ast && "program failed to produce AST"); 
	}
};

std::unique_ptr<my_ast_node> parse_pegtl(std::string input);

#endif
