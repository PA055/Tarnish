#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <string.h>
#include <strings.h>
#include <time.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif // DEBUG_PRINT_CODE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,    // = |= ^= &= >>= <<= += -= *= **= /= %= %%=
  PREC_TERNARY,       // ?:
  PREC_LOGICAL_OR,    // or ||
  PREC_LOGICAL_AND,   // and &&
  PREC_EQUALITY,      // == !=
  PREC_COMPARISON,    // < > <= >=
  PREC_BITWISE_OR,    // |
  PREC_BITWISE_XOR,   // ^
  PREC_BITWISE_AND,   // &
  PREC_BITWISE_SHIFT, // << >>
  PREC_TERM,          // + -
  PREC_FACTOR,        // * / % %%
  PREC_EXPONENT,      // **
  PREC_UNARY,         // ~ ! - +
  PREC_PREFIX,        // ++ --
  PREC_CALL,          // . () []
  PREC_POSTFIX,       // ++ --
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;
  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler *enclosing;
  bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = NULL;
ClassCompiler *currentClass = NULL;

static Chunk *currentChunk() { return &current->function->chunk; }

static void errorAt(Token *token, const char *message) {
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {

  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char *message) { errorAt(&parser.previous, message); }

static void errorAtCurrent(const char *message) {
  errorAt(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR)
      break;

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char *message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (!check(type))
    return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX)
    error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitBytes(0xff, 0xff);
  return currentChunk()->count - 2;
}

static void patchJump(int offset) {
  int jump = currentChunk()->count - offset - 2;
  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NONE);
  }

  emitByte(OP_RETURN);
}

static void emitConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant <= UINT8_MAX) {
    emitBytes(OP_CONSTANT, (uint8_t)constant);
  } else if (constant <= 0xffffff) {
    printf("CONSTANT_LONG\n");
    emitByte(OP_CONSTANT_LONG);
    emitByte(constant >> 0xf & 0xff);
    emitByte(constant >> 0x8 & 0xff);
    emitByte(constant >> 0x0 & 0xff);
  } else {
    error("Too many constants in one chunk.");
  }
}

static void initCompiler(Compiler *compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    current->function->name =
        copyString(parser.previous.start, parser.previous.length);
  }

  Local *local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != TYPE_FUNCTION) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static ObjFunction *endCompiler() {
  emitReturn();
  ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), function->name != NULL
                                         ? function->name->chars
                                         : "<script>");
  }
#endif // DEBUG_PRINT_CODE

  current = current->enclosing;
  return function;
}

static void beginScope() { current->scopeDepth++; }

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth > current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_KEYWORD_CLASS:
    case TOKEN_KEYWORD_FUNC:
    case TOKEN_KEYWORD_VAR:
    case TOKEN_KEYWORD_FOR:
    case TOKEN_KEYWORD_IF:
    case TOKEN_KEYWORD_WHILE:
    case TOKEN_KEYWORD_PRINT:
    case TOKEN_KEYWORD_RETURN:
      return;

    default:;
    }

    advance();
  }
}

static void parsePrecedence(Precedence precedence);
static ParseRule *getRule(TokenType type);
static void expression();
static void statement();
static void declaration();

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static uint8_t identifierConstant(Token *name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b) {
  if (a->length != b->length)
    return false;
  return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local *local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue *upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
  if (compiler->enclosing == NULL)
    return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

static void addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local *local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

static void declareVariable() {
  if (current->scopeDepth == 0)
    return;

  Token *name = &parser.previous;
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local *local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }
  addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0)
    return 0;

  return identifierConstant(&parser.previous);
}

static void markInitialized() {
  if (current->scopeDepth == 0)
    return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}

static void logical_and(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  parsePrecedence(PREC_LOGICAL_AND);
  patchJump(endJump);
}

static void logical_or(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_LOGICAL_OR);
  patchJump(endJump);
}

static void ternary(bool canAssign) {
  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  expression();
  int endJump = emitJump(OP_JUMP);
  consume(TOKEN_COLON, "Expect ':' in ternary.");
  patchJump(thenJump);
  parsePrecedence(PREC_TERNARY);
  patchJump(endJump);
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule *rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
  case TOKEN_BANG_EQUAL:
    emitBytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emitByte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emitByte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emitBytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emitByte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emitBytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_OR:
    emitByte(OP_OR);
    break;
  case TOKEN_XOR:
    emitByte(OP_XOR);
    break;
  case TOKEN_AND:
    emitByte(OP_AND);
    break;
  case TOKEN_LESS_LESS:
    emitByte(OP_LSHIFT);
    break;
  case TOKEN_GREATER_GREATER:
    emitByte(OP_RSHIFT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_SUBTRACT);
    break;
  case TOKEN_PLUS:
    emitByte(OP_ADD);
    break;
  case TOKEN_SLASH:
    emitByte(OP_DIVIDE);
    break;
  case TOKEN_STAR:
    emitByte(OP_MULTIPLY);
    break;
  case TOKEN_PERCENT:
    emitByte(OP_MODULUS);
    break;
  case TOKEN_PERCENT_PERCENT:
    emitByte(OP_FLOOR_DIVIDE);
    break;
  case TOKEN_STAR_STAR:
    emitByte(OP_EXPONENT);
    break;
  default:
    return;
  }
}

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'");
  uint8_t name = identifierConstant(&parser.previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
  case TOKEN_KEYWORD_FALSE:
    emitByte(OP_FALSE);
    break;
  case TOKEN_KEYWORD_NONE:
    emitByte(OP_NONE);
    break;
  case TOKEN_KEYWORD_TRUE:
    emitByte(OP_TRUE);
    break;
  default:
    return;
  }
}

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void float_(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(FLOAT_VAL(value));
}

static void int_(bool canAssign) {
  int value = strtol(parser.previous.start, NULL, 10);
  emitConstant(INT_VAL(value));
}

static void string(bool canAssign) {
  emitConstant(OBJ_VAL(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static Token syntheticToken(const char *text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    error("Can't use 'super' in a class with no superclass.");
  }

  consume(TOKEN_DOT, "Expect '.' after super.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'this' outside of a class");
    return;
  }

  variable(false);
}

static void list(bool canAssign) {
  int itemCount = 0;
  if (!check(TOKEN_RIGHT_BRACKET)) {
    do {
      if (check(TOKEN_RIGHT_BRACKET)) break;

      parsePrecedence(PREC_TERNARY);

      if (itemCount == UINT8_COUNT) {
        error("Cannot have more than 256 items in a list literal.");
      }
      itemCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after list literal.");

  emitByte(OP_LIST_BUILD);
  emitByte(itemCount);
}

static void subscript(bool canAssign) {
  parsePrecedence(PREC_TERNARY);
  consume(TOKEN_RIGHT_BRACKET, "Expect ']' after index.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_LIST_STORE);
  } else {
    emitByte(OP_LIST_INDEX);
  }
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  parsePrecedence(PREC_UNARY);

  switch (operatorType) {
  case TOKEN_BANG:
    emitByte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emitByte(OP_NEGATE);
    break;
  case TOKEN_TILDE:
    emitByte(OP_INVERT);
    break;
  default:
    return;
  }
}

ParseRule rules[] = {
    [TOKEN_AND_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, binary, PREC_BITWISE_AND},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_BANG] = {unary, NULL, PREC_UNARY},
    [TOKEN_COLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, dot, PREC_CALL},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FLOAT] = {float_, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_GREATER] = {NULL, binary, PREC_BITWISE_SHIFT},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_INT] = {int_, NULL, PREC_NONE},
    [TOKEN_KEYWORD_AND] = {NULL, logical_and, PREC_LOGICAL_AND},
    [TOKEN_KEYWORD_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_KEYWORD_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_FUNC] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_NONE] = {literal, NULL, PREC_NONE},
    [TOKEN_KEYWORD_OR] = {NULL, logical_or, PREC_LOGICAL_OR},
    [TOKEN_KEYWORD_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_SUPER] = {super_, NULL, PREC_NONE},
    [TOKEN_KEYWORD_THIS] = {this_, NULL, PREC_NONE},
    [TOKEN_KEYWORD_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_KEYWORD_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_KEYWORD_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACKET] = {list, subscript, PREC_CALL},
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_LESS] = {NULL, binary, PREC_BITWISE_SHIFT},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_MINUS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS_MINUS] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_OR_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, binary, PREC_BITWISE_OR},
    [TOKEN_PERCENT_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_PERCENT] = {NULL, binary, PREC_FACTOR},
    [TOKEN_PERCENT_PERCENT_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_PERCENT_PERCENT] = {NULL, binary, PREC_FACTOR},
    [TOKEN_PLUS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_PLUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS_PLUS] = {NULL, NULL, PREC_NONE},
    [TOKEN_QUESTION] = {NULL, ternary, PREC_TERNARY},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACKET] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR_STAR_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_STAR_STAR] = {NULL, binary, PREC_EXPONENT},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_TILDE] = {unary, NULL, PREC_UNARY},
    [TOKEN_XOR_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_XOR] = {NULL, binary, PREC_BITWISE_XOR},
};

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters");
      }
      uint8_t constant = parseVariable("Expect parameter name");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction *function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method() {
  consume(TOKEN_KEYWORD_FUNC, "Expect only methods in class body.");
  consume(TOKEN_IDENTIFIER, "Expect method name.");
  uint8_t constant = identifierConstant(&parser.previous);

  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 8 &&
      memcmp(parser.previous.start, "__init__", 8) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(type);
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.hasSuperclass = false;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;

  if (match(TOKEN_LEFT_PAREN)) {
    if (match(TOKEN_IDENTIFIER)) {
      variable(false);

      if (identifiersEqual(&className, &parser.previous)) {
        error("A class can't inherit from itself.");
      }

      beginScope();
      addLocal(syntheticToken("super"));
      defineVariable(0);

      namedVariable(className, false);
      emitByte(OP_INHERIT);
      classCompiler.hasSuperclass = true;
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after superclass.");
  }

  namedVariable(className, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  emitByte(OP_POP);

  if (classCompiler.hasSuperclass) {
    endScope();
  }

  currentClass = currentClass->enclosing;
}

static void funcDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NONE);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  defineVariable(global);
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }

    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value");
    emitByte(OP_RETURN);
  }
}

static void expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_POP);
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after conditions.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  int elseJump = emitJump(OP_JUMP);
  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_KEYWORD_ELSE))
    statement();
  patchJump(elseJump);
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void forStatement() {
  beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_SEMICOLON)) {
    // No initializer
  } else if (match(TOKEN_KEYWORD_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition");

    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP);
  }

  endScope();
}

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static void statement() {
  if (match(TOKEN_KEYWORD_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (match(TOKEN_KEYWORD_IF)) {
    ifStatement();
  } else if (match(TOKEN_KEYWORD_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_KEYWORD_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_KEYWORD_FOR)) {
    forStatement();
  } else {
    expressionStatement();
  }
}

static void declaration() {
  if (match(TOKEN_KEYWORD_CLASS)) {
    classDeclaration();
  } else if (match(TOKEN_KEYWORD_FUNC)) {
    funcDeclaration();
  } else if (match(TOKEN_KEYWORD_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode)
    synchronize();
}

ObjFunction *compile(const char *source) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction *function = endCompiler();
  return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
  Compiler *compiler = current;
  while (compiler != NULL) {
    markObject((Obj *)compiler->function);
    compiler = compiler->enclosing;
  }
}
