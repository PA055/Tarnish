#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "scanner.h"

typedef struct {
  const char *source;
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

static bool isAtEnd() { return *scanner.current == '\0'; }

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

static char peek() { return *scanner.current; }

static char peekNext() {
  if (isAtEnd())
    return '\0';
  return scanner.current[1];
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isAlphaNum(char c) { return isAlpha(c) || isDigit(c); }

static bool match(char expected) {
  if (isAtEnd())
    return false;
  if (*scanner.current != expected)
    return false;
  scanner.current++;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  return token;
}

static Token errorToken(const char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  return token;
}

void initScanner(const char *source) {
  scanner.source = source;
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
  if (peek() == '#' && peekNext() == '!') {
    while (peek() != '\n' && !isAtEnd())
      advance();
    scanner.line++;
  }
}

static void blockComment() {
  int depth = 0;
  do {
    if (peek() == '/' && peekNext() == '*')
      depth++;

    if (peek() == '*' && peekNext() == '/')
      depth--;

    if (peek() == '\n')
      scanner.line++;

    advance();
  } while (depth > 0);
  advance(); // the last /
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      advance();
      break;
    case '\n':
      scanner.line++;
      advance();
      break;
    case '/':
      if (peekNext() == '/') {
        while (peek() != '\n' && !isAtEnd())
          advance();
        } else if (peekNext() == '*') {
          blockComment();
        } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token string(char openQuote) {
  if (peek() == openQuote && peekNext() == openQuote) {
    for (;;) {
      char c = advance();
      if (c == '\n') scanner.line++;
      if (c == openQuote && peek() == openQuote && peekNext() == openQuote)
        break;
    }
    if (peek() != openQuote)
      return errorToken("Unterminated string.");

    advance();
    advance();
    advance();
  } else {
    while (peek() != openQuote && !isAtEnd()) {
      char c = advance();
      if (c == '\n') {
        scanner.line++;
        break;
      }
    }
    if (peek() != openQuote)
      return errorToken("Unterminated string.");

    advance();
  }

  return makeToken(TOKEN_STRING);
}

static Token number() {
  while (isDigit(peek()))
    advance();

  // Look for a fractional part.
  if (peek() == '.' && isDigit(peekNext())) {
    // Consume the ".".
    advance();

    while (isDigit(peek()))
      advance();
    return makeToken(TOKEN_FLOAT);
  }

  return makeToken(TOKEN_INT);
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (scanner.start[0]) {
  case 'a':
    return checkKeyword(1, 2, "nd", TOKEN_KEYWORD_AND);
  case 'c':
    return checkKeyword(1, 4, "lass", TOKEN_KEYWORD_CLASS);
  case 'e':
    return checkKeyword(1, 3, "lse", TOKEN_KEYWORD_ELSE);
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'a':
        return checkKeyword(2, 3, "lse", TOKEN_KEYWORD_FALSE);
      case 'o':
        return checkKeyword(2, 1, "r", TOKEN_KEYWORD_FOR);
      case 'u':
        return checkKeyword(2, 2, "nc", TOKEN_KEYWORD_FUNC);
      }
    }
    break;
  case 'i':
    return checkKeyword(1, 1, "f", TOKEN_KEYWORD_IF);
  case 'n':
    return checkKeyword(1, 3, "one", TOKEN_KEYWORD_NONE);
  case 'o':
    return checkKeyword(1, 1, "r", TOKEN_KEYWORD_OR);
  case 'p':
    return checkKeyword(1, 4, "rint", TOKEN_KEYWORD_PRINT);
  case 'r':
    return checkKeyword(1, 5, "eturn", TOKEN_KEYWORD_RETURN);
  case 's':
    return checkKeyword(1, 4, "uper", TOKEN_KEYWORD_SUPER);
  case 't':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return checkKeyword(2, 2, "is", TOKEN_KEYWORD_THIS);
      case 'r':
        return checkKeyword(2, 2, "ue", TOKEN_KEYWORD_TRUE);
      }
    }
    break;
  case 'v':
    return checkKeyword(1, 2, "ar", TOKEN_KEYWORD_VAR);
  case 'w':
    return checkKeyword(1, 4, "hile", TOKEN_KEYWORD_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlphaNum(peek()))
    advance();
  return makeToken(identifierType());
}

static Token getToken() {
  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd())
    return makeToken(TOKEN_EOF);

  if (peek() == '.' && isDigit(peekNext()))
    return number();

  char c = advance();
  if (isAlpha(c))
    return identifier();
  if (isDigit(c))
    return number();

  switch (c) {
  case '(':
    return makeToken(TOKEN_LEFT_PAREN);
  case ')':
    return makeToken(TOKEN_RIGHT_PAREN);
  case '{':
    return makeToken(TOKEN_LEFT_BRACE);
  case '}':
    return makeToken(TOKEN_RIGHT_BRACE);
  case '[':
    return makeToken(TOKEN_LEFT_BRACKET);
  case ']':
    return makeToken(TOKEN_RIGHT_BRACKET);
  case ';':
    return makeToken(TOKEN_SEMICOLON);
  case ',':
    return makeToken(TOKEN_COMMA);
  case '.':
    return makeToken(TOKEN_DOT);
  case '?':
    return makeToken(TOKEN_QUESTION);
  case ':':
    return makeToken(TOKEN_COLON);
  case '~':
    return makeToken(TOKEN_TILDE);

  case '/':
    return makeToken(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
  case '^':
    return makeToken(match('=') ? TOKEN_XOR_EQUAL : TOKEN_XOR);
  case '!':
    return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);

  case '&':
    return makeToken(match('=')   ? TOKEN_AND_EQUAL
                     : match('&') ? TOKEN_KEYWORD_AND
                                  : TOKEN_AND);
  case '|':
    return makeToken(match('=')   ? TOKEN_OR_EQUAL
                     : match('|') ? TOKEN_KEYWORD_OR
                                  : TOKEN_OR);
  case '-':
    return makeToken(match('=')   ? TOKEN_MINUS_EQUAL
                     : match('-') ? TOKEN_MINUS_MINUS
                                  : TOKEN_MINUS);
  case '+':
    return makeToken(match('=')   ? TOKEN_PLUS_EQUAL
                     : match('+') ? TOKEN_PLUS_PLUS
                                  : TOKEN_PLUS);

  case '*':
    if (match('*'))
      return makeToken(match('=') ? TOKEN_STAR_STAR_EQUAL : TOKEN_STAR_STAR);
    return makeToken(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
  case '%':
    if (match('%'))
      return makeToken(match('=') ? TOKEN_PERCENT_PERCENT_EQUAL
                                  : TOKEN_PERCENT_PERCENT);
    return makeToken(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
  case '<':
    if (match('<'))
      return makeToken(match('=') ? TOKEN_LESS_LESS_EQUAL : TOKEN_LESS_LESS);
    return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    if (match('>'))
      return makeToken(match('=') ? TOKEN_GREATER_GREATER_EQUAL
                                  : TOKEN_GREATER_GREATER);
    return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

  case '\'':
  case '"':
    return string(c);
  }

  return errorToken("Unexpected character.");
}

Token scanToken() {
  Token t = getToken();
#ifdef DEBUG_PRINT_SCANNING
  char *chars = malloc(sizeof(char) * t.length + 1);
  memcpy((void *)chars, t.start, t.length);
  chars[t.length] = '\0';
  printf("Token of type %u ('%s') at file:%u, char %td\n", t.type, chars,
         t.line, t.start - scanner.source);
#endif // DEBUG_PRINT_SCANNING
  return t;
}
