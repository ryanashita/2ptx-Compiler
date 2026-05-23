#include "parser.hpp"
#include <iostream>

std::unique_ptr<my_ast_node> parse_pegtl(std::string input) {
	std::cout << "=== PARSE PEGTL ===" << std::endl; 

	pegtl::memory_input in(input, "input"); 
	auto root = pegtl::parse_tree::parse< grammar::program,my_ast_node,selector>( in );

	// std::cout << "\n=== END PARSE PEGTL ===\n" << std::endl; 
	return root ? std::move(root) : nullptr;
	/*
		if (root) return std::move(root)
		else return nullptr 
	*/
}