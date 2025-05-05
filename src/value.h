#ifndef tarnish_value_h
#define tarnish_value_h

#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

typedef uint64_t Value;

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)
#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3
#define TAG_INT 4
#define TAG_RESERVED1 5
#define TAG_RESERVED2 6
#define TAG_RESERVED3 7

#define FALSE_VAL ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NONE_VAL ((Value)(uint64_t)(QNAN | TAG_NIL))

#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define INT_VAL(i) ((Value)(QNAN | ((uint64_t)(uint32_t)(i) << 3) | TAG_INT))
#define FLOAT_VAL(num) numToValue(num)
#define NUMBER_VAL(num) numToValue(num)

#ifdef DEBUG_ASSERTS
#define OBJ_VAL(obj) objToValue((Obj *)obj)
static inline Value objToValue(Obj *obj) {
  ASSERT(((SIGN_BIT | QNAN) & (uintptr_t)(obj)) == 0,
         "Pointer getting overwritten by QNAN bits");
  return ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)));
}
#else // !DEBUG_ASSERTS
#define OBJ_VAL(obj) ((Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj)))
#endif // DEBUG_ASSERTS

#define IS_NONE(value) ((value) == NONE_VAL)
#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_INT(value) valueIsInt(value)
#define IS_FLOAT(value) (((value) & QNAN) != QNAN)
#define IS_NUMBER(value) valueIsNum(value)
#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value) ((value) == TRUE_VAL)
#define AS_INT(value) ((int)(value >> 3))
#define AS_FLOAT(value) valueToFloat(value)
#define AS_NUMBER(value) valueToNum(value)
#define AS_OBJ(value) ((Obj *)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

static inline bool valueIsInt(Value value) {
  return (value & (QNAN | TAG_INT)) == (QNAN | TAG_INT) &&
         ((value | SIGN_BIT) != value);
}

static inline bool valueIsNum(Value value) {
  return IS_FLOAT(value) || valueIsInt(value);
}

static inline Value numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

static inline double valueToFloat(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline double valueToNum(Value value) {
  return IS_FLOAT(value) ? AS_FLOAT(value) : AS_INT(value);
}

#else // !NAN_BOXING

typedef enum {
  VAL_BOOL,
  VAL_NONE,
  VAL_INT,
  VAL_FLOAT,
  VAL_OBJ,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    int ival;
    double fval;
    Obj *obj;
  } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NONE(value) ((value).type == VAL_NONE)
#define IS_INT(value) ((value).type == VAL_INT)
#define IS_FLOAT(value) ((value).type == VAL_FLOAT)
#define IS_NUMBER(value) valueIsNum(value)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_INT(value) ((value).as.ival)
#define AS_FLOAT(value) ((value).as.fval)
#define AS_NUMBER(value) valueToNum(value)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NONE_VAL ((Value){VAL_NONE, {.fval = 0}})
#define INT_VAL(value) ((Value){VAL_INT, {.ival = value}})
#define FLOAT_VAL(value) ((Value){VAL_FLOAT, {.fval = value}})
#define NUMBER_VAL(value) ((Value){VAL_FLOAT, {.fval = value}})
#define OBJ_VAL(value) ((Value){VAL_OBJ, {.obj = (Obj *)value}})

static inline bool valueIsNum(Value value) {
  return IS_FLOAT(value) || IS_INT(value);
}

static inline double valueToNum(Value value) {
  return IS_INT(value) ? AS_INT(value) : AS_FLOAT(value);
}

#endif // NAN_BOXING

typedef struct {
  int capacity;
  int count;
  Value *values;
} ValueArray;

void initValueArray(ValueArray *array);
void writeValueArray(ValueArray *array, Value value);
void freeValueArray(ValueArray *array);

bool valuesEqual(Value a, Value b);
void printValue(Value value);

#endif // !tarnish_value_h
