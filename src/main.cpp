#include "main.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <sysexits.h>
#include <vector>

#include "astprinter.hpp"
#include "expr.hpp"
#include "scanner.hpp"
#include "token.hpp"

bool hadError = true;

void report(int line, std::string where, std::string message) {
    std::cerr << "[line " << line << "] Error " << where << ": " << message << std::endl; 
    hadError = true;
}

void error(int line, std::string message) { report(line, "", message); }

int run(std::string code) {
    Scanner scanner{code};
    std::vector<Token> tokens = scanner.scanTokens();

    for (Token token : tokens) {
        std::cout << token << std::endl;    
    }
    return 0;
}


int runFile(std::string filename) {
    std::ifstream file{filename};
    if (!file.is_open()) {
        std::cerr << "Error reading file: " << filename << std::endl;
        return 1;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    file.close();

    int err = run(buf.str());
    if (hadError) err = 1;
    return err;
}


int runPrompt() {
    std::string line;
    while (true) {
        std::cout << ">>> " << std::flush;
        if (!std::getline(std::cin, line)) return 0;
        run(line);
        hadError = false;
    }
    return 0;
}


int main(const int argc, char** argv) {
    if (argc > 2) {
        std::cout << "Usage: tarnish [script]" << std::endl;
        return EX_USAGE;
    } else if (argc == 2) {
        if (!strcmp(argv[1], "--astTest")) {
            std::shared_ptr<Expr<std::string>> literal1 = std::make_shared<Literal<std::string>>(Object(123));
            std::shared_ptr<Expr<std::string>> literal2 = std::make_shared<Literal<std::string>>(Object(45.67));
            std::shared_ptr<Expr<std::string>> unary = std::make_shared<Unary<std::string>>(Token(TokenType::MINUS, 1, "-", Object()), literal1);
            std::shared_ptr<Expr<std::string>> grouping = std::make_shared<Grouping<std::string>>(literal2);
            std::shared_ptr<Expr<std::string>> expr = std::make_shared<Binary<std::string>>(
                unary,
                Token(TokenType::STAR, 1, "*", Object()),
                grouping
            );
            std::cout << std::make_shared<AstPrinter>()->print(expr) << std::endl;
            return 0;
        }
        return runFile(argv[1]);
    } else {
        return runPrompt();
    }
}
