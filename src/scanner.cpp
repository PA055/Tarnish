#include "scanner.hpp"

#include <string>
#include <unordered_map>
#include <vector>

#include "main.hpp"
#include "token.hpp"

void Scanner::addToken(TokenType type) {
    addToken(type, Object());
}

void Scanner::addToken(TokenType type, Object obj) {
    std::string text = source.substr(start, current - start);
    tokens.push_back({type, line, text, obj});
}

bool Scanner::isAtEnd() {
    return current >= source.length();
}

bool Scanner::isDigit(char c) {
   return c >= '0' && c <= '9'; 
}

bool Scanner::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
}

bool Scanner::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

char Scanner::advance() {
    return source.at(current++);
}

char Scanner::peek() {
    if (isAtEnd()) return '\0';
    return source.at(current);
}

char Scanner::peek(int count) {
    if (current + count >= source.length()) return '\0';
    return source.at(current + count);
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source.at(current) != expected) return false;

    current++;
    return true;
}


void Scanner::identifier() {
    while (isAlphaNumeric(peek())) advance();

    static const std::unordered_map<std::string, TokenType> keywordMap = {
        {"and", AND}, {"class", CLASS}, {"else", ELSE}, {"false", FALSE},
        {"func", FUNC}, {"for", FOR}, {"if", IF}, {"none", NONE}, {"or", OR},
        {"print", PRINT}, {"return", RETURN}, {"super", SUPER}, {"this", THIS},
        {"true", TRUE}, {"var", VAR}, {"while", WHILE}
    };

    std::string str = source.substr(start, current - start);
    auto it = keywordMap.find(str);
    if (it != keywordMap.end()) {
        addToken(it->second);
        return;
    }

    addToken(IDENTIFIER);
} 

void Scanner::number() {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peek(1))) {
        advance();
        while (isDigit(peek())) advance();

        std::string val = source.substr(start, current - start);
        Object value = Object(std::stod(val));
        addToken(TokenType::NUMBER, value);
        return;
    }

    std::string val = source.substr(start, current - start);
    Object value = Object(std::stoi(val));
    addToken(TokenType::NUMBER, value);
    return;
}

void Scanner::string() { // TODO: make normal strings vs multi line strings
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line++;
        advance();
    }
    if (isAtEnd()) {
        error(line, "Unterminated string");
        return;
    }
    advance();

    Object value = Object(source.substr(start + 1, current - start - 2));
    addToken(TokenType::STRING, value);
}

void Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case ':': addToken(TokenType::COLON); break;
        case '@': addToken(TokenType::AT_SIGN); break;
        case '?': addToken(TokenType::QUESTION_MARK); break;

        case '!':
            addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
            break;
        case '=':
            addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
            break;
        case '<':
            addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
            break;
        case '>':
            addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
            break;
        case '%':
            addToken(match('=') ? TokenType::PERCENT_EQUAL : TokenType::PERCENT);
            break;
        case '^':
            addToken(match('=') ? TokenType::CARET_EQUAL : TokenType::CARET);
            break;
        case '~':
            addToken(match('=') ? TokenType::TILDE_EQUAL : TokenType::TILDE);
            break;

        case '*':
            addToken(match('*') ? TokenType::STAR_STAR :
                     match('=') ? TokenType::STAR_EQUAL :
                                  TokenType::STAR);
            break;

        case '&':
            addToken(match('&') ? TokenType::AMPERSAND_AMPERSAND :
                     match('=') ? TokenType::AMPERSAND_EQUAL :
                                  TokenType::AMPERSAND);
            break;

        case '|':
            addToken(match('|') ? TokenType::BAR_BAR :
                     match('=') ? TokenType::BAR_EQUAL :
                                  TokenType::BAR);
            break;

        case '+':
            addToken(match('+') ? TokenType::PLUS_PLUS :
                     match('=') ? TokenType::PLUS_EQUAL :
                                  TokenType::PLUS);
            break;
            
        case '-':
            addToken(match('-') ? TokenType::MINUS_MINUS :
                     match('>') ? TokenType::ARROW :
                     match('=') ? TokenType::MINUS_EQUAL :
                                  TokenType::MINUS);
            break;

        case '/':
            if (match('/'))
                while (peek() != '\n' && !isAtEnd())
                    advance();
            else
                addToken(match('=') ? TokenType::SLASH_EQUAL : TokenType::SLASH);
            break;

        case '.': 
            if (isDigit(peek())) {
                current--;
                number();
            } else addToken(TokenType::DOT);
            break;

        case '"': string(); break;

        case ' ':
        case '\r':
        case '\t':
            break;

        case '\n':
            line++;
            break;

        default:
            if (isDigit(c)) number();
            else if (isAlpha(c)) identifier();
            else error(line, "Unexpected character.");
            break;
    }
}

std::vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    tokens.push_back({TokenType::END_OF_FILE, line, "", Object()});
    return tokens;
}
