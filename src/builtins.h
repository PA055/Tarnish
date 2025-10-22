#ifndef tarnish_builtins_h
#define tarnish_builtins_h

#include "object.h"

NativeResult timeNative(int argCount, Value* args);

NativeResult strNative(int argCount, Value* args);
NativeResult intNative(int argCount, Value* args);

#endif // !tarnish_builtins_h
