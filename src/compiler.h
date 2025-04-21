#ifndef tarnish_compiler_h
#define tarnish_compiler_h

#include "object.h"
#include "vm.h"
#include "chunk.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif // !tarnish_compiler_h
