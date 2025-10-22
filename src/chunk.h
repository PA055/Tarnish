#ifndef tarnish_chunk_h
#define tarnish_chunk_h

#include "value.h"

typedef enum {
    OP_ADD,
    OP_AND,
    OP_CALL,
    OP_CLASS,
    OP_CLOSE_UPVALUE,
    OP_CLOSURE,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_DEFINE_GLOBAL,
    OP_DIVIDE,
    OP_EQUAL,
    OP_EXPONENT,
    OP_FALSE,
    OP_FLOOR_DIVIDE,
    OP_GET_GLOBAL,
    OP_GET_LOCAL,
    OP_GET_PROPERTY,
    OP_GET_SUPER,
    OP_GET_UPVALUE,
    OP_GREATER,
    OP_INHERIT,
    OP_INVERT,
    OP_INVOKE,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LESS,
    OP_LIST_BUILD,
    OP_LIST_INDEX,
    OP_LIST_STORE,
    OP_LOOP,
    OP_LSHIFT,
    OP_METHOD,
    OP_MODULUS,
    OP_MULTIPLY,
    OP_NEGATE,
    OP_NONE,
    OP_NOT,
    OP_OR,
    OP_POP,
    OP_PRINT,
    OP_RETURN,
    OP_RSHIFT,
    OP_SET_GLOBAL,
    OP_SET_LOCAL,
    OP_SET_PROPERTY,
    OP_SET_UPVALUE,
    OP_SUBTRACT,
    OP_SUPER_INVOKE,
    OP_TRUE,
    OP_XOR,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t *code;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void writeConstant(Chunk* chunk, Value value, int line);
int addConstant(Chunk* chunk, Value value);

#endif // !tarnish_chunk_h
