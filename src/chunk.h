#ifndef tarnish_chunk_h
#define tarnish_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    OP_ADD,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_DEFINE_GLOBAL,
    OP_DIVIDE,
    OP_EQUAL,
    OP_FALSE,
    OP_GET_GLOBAL,
    OP_GET_LOCAL,
    OP_GREATER,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LESS,
    OP_MULTIPLY,
    OP_NEGATE,
    OP_NONE,
    OP_NOT,
    OP_POP,
    OP_PRINT,
    OP_RETURN,
    OP_SET_GLOBAL,
    OP_SET_LOCAL,
    OP_SUBTRACT,
    OP_TRUE,
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
