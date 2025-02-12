#pragma once

#include <string>
#include <vector>

#include "token.hpp"

class Scanner {
public:
    Scanner(std::string source) : source(source) {}

    std::vector<Token> scanTokens();

private:
    void scanToken();
    void addToken(TokenType type);
    void addToken(TokenType type, Object obj);

    bool isAtEnd();
    bool isDigit(char c);
    bool isAlpha(char c);
    bool isAlphaNumeric(char c);

    char advance();
    char peek();
    char peek(int count);
    bool match(char expected);

    void string();
    void number();
    void identifier();

    std::string source;
    std::vector<Token> tokens{};
    int start, current = 0;
    int line = 1;

};
