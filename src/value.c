#include <stdbool.h>
#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void initValueArray(ValueArray *array) {
  array->values = NULL;
  array->capacity = 0;
  array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
  if (array->capacity < array->count + 1) {
    int oldCapacity = array->capacity;
    array->capacity = GROW_CAPACITY(oldCapacity);
    array->values =
        GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
  }

  array->values[array->count] = value;
  array->count++;
}

void freeValueArray(ValueArray *array) {
  FREE_ARRAY(Value, array->values, array->capacity);
  initValueArray(array);
}

bool valuesEqual(Value a, Value b) {
#ifdef NAN_BOXING
  if (IS_NUMBER(a) && IS_NUMBER(b)) {
    return AS_NUMBER(a) == AS_NUMBER(b);
  }
  return a == b;
#else  // !NAN_BOXING
  if (a.type != b.type)
    return false;

  switch (a.type) {
  case VAL_BOOL:
    return AS_BOOL(a) == AS_BOOL(b);
  case VAL_NONE:
    return true;
  case VAL_INT:
    return AS_INT(a) == AS_INT(b);
  case VAL_FLOAT:
    return AS_FLOAT(a) == AS_FLOAT(b);
  case VAL_OBJ:
    return AS_OBJ(a) == AS_OBJ(b);
  default:
    return false;
  }
#endif // NAN_BOXING
}

void printValue(Value value) {
#ifdef NAN_BOXING
  if (IS_BOOL(value)) {
    printf(AS_BOOL(value) ? "true" : "false");
  } else if (IS_NONE(value)) {
    printf("none");
  } else if (IS_INT(value)) {
    printf("%i", AS_INT(value));
  } else if (IS_FLOAT(value)) {
    printf("%f", AS_FLOAT(value));
  } else if (IS_OBJ(value)) {
    printObject(value);
  }
#else  // !NAN_BOXING
  switch (value.type) {
  case VAL_BOOL:
    printf(AS_BOOL(value) ? "true" : "false");
    break;
  case VAL_NONE:
    printf("none");
    break;
  case VAL_INT:
    printf("%i", AS_INT(value));
    break;
  case VAL_FLOAT:
    printf("%f", AS_FLOAT(value));
    break;
  case VAL_OBJ:
    printObject(value);
    break;
  }
#endif // NAN_BOXING
}
