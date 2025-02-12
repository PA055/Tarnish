#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>

#include "lib/magic_enum.hpp"

enum TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, 
    LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, SEMICOLON, 
    COLON, AT_SIGN, QUESTION_MARK, 

    // One or two character tokens.
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    LESS, LESS_EQUAL,
    GREATER, GREATER_EQUAL,
    PERCENT, PERCENT_EQUAL,
    CARET, CARET_EQUAL,
    TILDE, TILDE_EQUAL,
    SLASH, SLASH_EQUAL,
    BAR, BAR_BAR, BAR_EQUAL,
    STAR, STAR_STAR, STAR_EQUAL,
    PLUS, PLUS_EQUAL, PLUS_PLUS,
    AMPERSAND, AMPERSAND_AMPERSAND, AMPERSAND_EQUAL,
    MINUS, MINUS_EQUAL, MINUS_MINUS, ARROW,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE, FUNC, FOR, IF, NONE, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

class None {};
inline bool operator==(None, None) { return true; }

class Object {
public:
    Object(std::string val) : mData(val) {}
    Object(int val) : mData(val) {}
    Object(double val) : mData(val) {}
    Object(bool val) : mData(val) {}
    Object() : mData(None{}) {}

    std::string __str__() const;

    bool isString() const;
    bool isDouble() const;
    bool isInteger() const;
    bool isBool() const;
    bool isNone() const;
    
    friend bool operator==(const Object& lhs, const Object& rhs);
    friend std::ostream& operator<<(std::ostream& os, const Object& obj);
private:
    std::variant<std::string, int, double, bool, None> mData;
};


class Token {
public:
    Token(TokenType type, int line, std::string lexme, Object literal) : 
        type(type), line(line), lexme(std::move(lexme)), literal(literal) {}

    friend std::ostream& operator<<(std::ostream& os, const Token& obj) {
        os << magic_enum::enum_name(obj.type) << " on line " << obj.line << ": " << obj.lexme; 
        if (!obj.literal.isNone()) {
            os << "(" << obj.literal << ")";
        }
        return os;
    }

    TokenType type;
    int line;
    std::string lexme; 
    Object literal;
};
