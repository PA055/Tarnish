#ifndef tarnish_compiler_h
#define tarnish_compiler_h

#include "object.h"
#include "vm.h"
#include "chunk.h"

bool compile(const char* source, Chunk* chunk);

#endif // !tarnish_compiler_h
