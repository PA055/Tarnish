#include "builtins.h"
#include "object.h"
#include "value.h"

#include <memory.h>
#include <time.h>

NativeResult timeNative(int argCount, Value* args) {
    return (NativeResult){false, NUMBER_VAL((double)clock() / CLOCKS_PER_SEC)};
}
