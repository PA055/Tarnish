#include "builtins.h"
#include "object.h"
#include "value.h"

#include <memory.h>
#include <stdbool.h>
#include <time.h>

NativeResult timeNative(int argCount, Value* args) {
    return (NativeResult) {false, NUMBER_VAL((double)clock() / CLOCKS_PER_SEC)};
}

NativeResult strNative(int argCount, Value* args) {
    if (IS_INT(args[0])) {
        char buffer[32];
        int len = snprintf(buffer, sizeof(buffer), "%d", AS_INT(args[0]));
        return (NativeResult) {false, OBJ_VAL(copyString(buffer, len))};
    } else if (IS_NUMBER(args[0])) {
        char buffer[128];
        int len = snprintf(buffer, sizeof(buffer), "%.8f", AS_NUMBER(args[0]));
        return (NativeResult) {false, OBJ_VAL(copyString(buffer, len))};
    } else if (IS_BOOL(args[0])) {
        char* value = AS_BOOL(args[0]) ? "true" : "false";
        return (NativeResult) {false, OBJ_VAL(copyString(value, AS_BOOL(args[0]) ? 4 : 5))};
    }
    return (NativeResult) {true, INT_VAL(0)};
}

NativeResult intNative(int argCount, Value* args) {
    if (IS_NUMBER(args[0])) { return (NativeResult) {false, INT_VAL((int)AS_NUMBER(args[0]))}; }
    return (NativeResult) {true, INT_VAL(0)};
}
