#include <catch2/catch_test_macros.hpp>
#include "parser.hpp"
#include "ast.hpp"

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
namespace pegtl = tao::pegtl;

/*
    x = 1; [3,3,3]; 1  + (-2);
*/

TEST_CASE("Parse Integer", "[parser][numbers]") {
    auto root = parse_pegtl("44"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[44]");
    //"ExprList[Integer(44)]"
}

TEST_CASE("Parse Positive Integer", "[parser][numbers]") {
    auto root = parse_pegtl("+4"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[4]");
}

TEST_CASE("Parse Negative Integer", "[parser][numbers]") {
    auto root = parse_pegtl("-26"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[-26]");
}

TEST_CASE("Parse Addition", "[parser][arithmetic]") {
    auto root = parse_pegtl("1+1"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[(1 + 1)]");
}

TEST_CASE("Parse Subtraction", "[parser][arithmetic]") {
    auto root = parse_pegtl("17-5"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[(17 - 5)]");
}

TEST_CASE("Parse Multiplication", "[parser][arithmetic]") {
    auto root = parse_pegtl("2*9"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[(2 * 9)]");
}

TEST_CASE("Parse Division", "[parser][arithmetic]") {
    auto root = parse_pegtl("50/10"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[(50 / 10)]"); 
}

TEST_CASE("Parse Parentheses Addition ", "[parser][arithmetic][parentheses]") {
    auto root = parse_pegtl("(-1+5)+(2+(+3))"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[((-1 + 5) + (2 + 3))]");
}

TEST_CASE("Parse Parentheses Whitespace Addition ", "[parser][arithmetic][parentheses][whitespace]") {
    auto root = parse_pegtl("(-1  + 5) +  (2+ +3  )"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[((-1 + 5) + (2 + 3))]");
}

TEST_CASE("Parse Vector ", "[parser][vector]") {
    auto root = parse_pegtl("[1,2,3,4,5]"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[[1,2,3,4,5]]");
}

TEST_CASE("Parse Assignment ", "[parser][assignment][numbers]") {
    auto root = parse_pegtl("x = 1"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = 1]");
}

TEST_CASE("Parse Addition Assignment ", "[parser][assignment][numbers]") {
    auto root = parse_pegtl("x = (1+1+1-1)"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = (((1 + 1) + 1) - 1)]");
}

TEST_CASE("Parse Vector Assignment ", "[parser][assignment][vector]") {
    auto root = parse_pegtl("x = [1,2,3,4]"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = [1,2,3,4]]");
}

TEST_CASE("Parse Addition Whitespace Vector", "[parser][arithmetic][whitespace][vector]") {
    auto root = parse_pegtl("(-1  + 5) +  (2+ +3  ); [1,2];"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[((-1 + 5) + (2 + 3)),[1,2]]");
}

TEST_CASE("Parse Vector Vector", "[parser][vector]") {
    auto root = parse_pegtl("[1,2]; [3,4];"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[[1,2],[3,4]]");
}

TEST_CASE("Parse Addition Assignment Addition Integer", "[parser][arithmetic][number]") {
    auto root = parse_pegtl("x = (1+1+1-1); (5 + 5); 3;"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = (((1 + 1) + 1) - 1),(5 + 5),3]");
}

TEST_CASE("Parse Assignment Vector Addition", "[parser][vector][assignment][arithmetic]") {
    auto root = parse_pegtl("x = 1; [3,3,3]; 1  + (-2);"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = 1,[3,3,3],(1 + -2)]");
}

// fun test case with variables and addition
TEST_CASE("Parse Assignment Variable Addition Reassignment", "[parser][assignment][variable][arithmetic]") {
    auto root = parse_pegtl("x = 1; x = (x + x); x;"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 

    std::string print_res = my_root->ast->print_line(); 
    REQUIRE(print_res == "[x = 1,x = (x + x),x]");
}

TEST_CASE("Parse Emit Three Address Code Constant String", "[parser][three_address_code][numbers]") {
    auto root = parse_pegtl("17"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 17"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}

TEST_CASE("Parse Emit Three Address Code Arithmetic", "[parser][three_address_code][arithmetic]") {
    auto root = parse_pegtl("4 + 5"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 4",
        "t2 <- 5",
        "t3 <- t1 + t2"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}

TEST_CASE("Parse Emit Three Address Code Arithmetic 2", "[parser][three_address_code][arithmetic]") {
    auto root = parse_pegtl("2 + 2"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 2",
        "t2 <- t1 + t1"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}

TEST_CASE("Parse Emit Three Address Code Multiplication", "[parser][three_address_code][arithmetic]") {
    auto root = parse_pegtl("54 * 8"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 54",
        "t2 <- 8",
        "t3 <- t1 * t2"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}

// testing Integer, Arithmetic, and Variable parsing and three-address code generation
TEST_CASE("Parse Emit TAC Arithmetic Assignment", "[parser][three_address_code][arithmetic][assignment]") {
    auto root = parse_pegtl("x = (1 + 2)"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 1",
        "t2 <- 2",
        "t3 <- t1 + t2",
        "x1 <- t3"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}

TEST_CASE("Parse Emit TAC Complex 1", "[parser][three_address_code]") {
    auto root = parse_pegtl("x = (2 + 3); 6; y = x * 9"); 
    REQUIRE(root);

    REQUIRE(!root->children.empty()); 
    my_ast_node* my_root = static_cast<my_ast_node*>(root->children[0].get()); 
    REQUIRE(my_root->ast != nullptr); 
    my_root->ast->three_address_code(); 
    const auto& tac_desired = my_root->ast->tac_nodes; // get the tac_lines vector
    std::vector tac_actual = 
    {
        "t1 <- 2",
        "t2 <- 3",
        "t3 <- t1 + t2",
        "x1 <- t3",
        "t4 <- 6",
        "t5 <- x1",
        "t6 <- 9",
        "t7 <- t5 * t6",
        "y1 <- t7"
    };

    // match size of desired and actual vector, and check they are equal at each index
    REQUIRE(tac_desired.size() == tac_actual.size());
    for (size_t i = 0; i < tac_desired.size(); ++i) {
        REQUIRE(tac_desired[i]->to_string() == tac_actual[i]); 
    }
}