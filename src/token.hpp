#pragma once

#include <memory>
#include <ostream>

#include "lib/magic_enum.hpp"
#include "objects/object.hpp"

enum TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, 
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, SEMICOLON, 
    COLON, CARET, AT_SIGN,
    QUESTION_MARK, 

    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    LESS, LESS_EQUAL,
    GREATER, GREATER_EQUAL,
    SLASH, SLASH_EQUAL,
    STAR, STAR_STAR, STAR_EQUAL,
    PLUS, PLUS_EQUAL, PLUS_PLUS,
    MINUS, MINUS_EQUAL, MINUS_MINUS, ARROW,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE, FUNC, FOR, IF, NONE, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

class Token {
public:
    Token(TokenType type, int line, std::string lexme, std::shared_ptr<Object> literal) : 
        type(type), line(line), lexme(std::move(lexme)), literal(literal) {}

    friend std::ostream& operator<<(std::ostream& os, const Token& obj) {
        os << magic_enum::enum_name(obj.type) << " on line " << obj.line << ": " << obj.lexme; 
        if (obj.literal) {
            os << "(" << *obj.literal << ")";
        }
        return os;
    }

private:
    TokenType type;
    int line;
    std::string lexme; 
    std::shared_ptr<Object> literal;
};
