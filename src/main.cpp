#include "main.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sysexits.h>
#include <vector>

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
        return runFile(argv[1]);
    } else {
        return runPrompt();
    }
}
