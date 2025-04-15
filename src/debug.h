#ifndef tarnish_debug_h
#define tarnish_debug_h

#include "chunk.h"

void disassembleChunk(Chunk* chunk, const char* name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif // !tarnish_debug_h
