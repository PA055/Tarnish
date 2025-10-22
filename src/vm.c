#include "vm.h"
#include "builtins.h"
#include "chunk.h"
#include "common.h"
#include "compiler.h"
#include "list.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif // DEBUG_TRACE_EXECUTION

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

VM vm;

static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void runtimeError(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame *frame = &vm.frames[i];
    ObjFunction *function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;

    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "<func %s>\n", function->name->chars);
    }
  }

  resetStack();
}

static void defineNative(const char *name, NativeFn function, int arity) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function, arity)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  resetStack();
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;

  initTable(&vm.globals);
  initTable(&vm.strings);

  vm.initString = NULL;
  vm.initString = copyString("__init__", 8);

  defineNative("time", timeNative, 0);
  defineNative("str", strNative, 1);
  defineNative("int", intNative, 1);
}

void freeVM() {
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  vm.initString = NULL;
  freeObjects();
}

void push(Value value) {
  *vm.stackTop = value;
  vm.stackTop++;
}

Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}

Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool call(ObjClosure *closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.", closure->function->arity,
                 argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame *frame = &vm.frames[vm.frameCount++];
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}

static bool nativeCall(ObjNative *native, int argCount) {
  if (argCount != native->arity) {
    runtimeError("Expected %d arguments but got %d.", native->arity, argCount);
    return false;
  }

  NativeResult result = native->function(argCount, vm.stackTop - argCount);
  vm.stackTop -= argCount + 1;

  if (result.error)
    return false;
  push(result.result);
  return true;
}

static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_BOUND_METHOD: {
      ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
      vm.stackTop[-argCount - 1] = bound->receiver;
      return call(bound->method, argCount);
    }
    case OBJ_CLASS: {
      ObjClass *klass = AS_CLASS(callee);
      vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));

      Value initializer;
      if (tableGet(&klass->methods, vm.initString, &initializer)) {
        return call(AS_CLOSURE(initializer), argCount);
      } else if (argCount != 0) {
        runtimeError("Expected 0 arguments but got %d.", argCount);
        return false;
      }

      return true;
    }
    case OBJ_CLOSURE:
      return call(AS_CLOSURE(callee), argCount);
    case OBJ_NATIVE:
      return nativeCall(AS_NATIVE(callee), argCount);
    default:
      break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

static bool invokeFromClass(ObjClass *klass, ObjString *name, int argCount) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  return call(AS_CLOSURE(method), argCount);
}

static bool invoke(ObjString *name, int argCount) {
  Value receiver = peek(argCount);

  if (!IS_INSTANCE(receiver)) {
    runtimeError("Only instances have methods.");
    return false;
  }

  ObjInstance *instance = AS_INSTANCE(receiver);

  Value value;
  if (tableGet(&instance->fields, name, &value)) {
    vm.stackTop[-argCount - 1] = value;
    return callValue(value, argCount);
  }

  return invokeFromClass(instance->klass, name, argCount);
}

static bool bindMethod(ObjClass *klass, ObjString *name) {
  Value method;
  if (!tableGet(&klass->methods, name, &method)) {
    runtimeError("Undefined property '%s'.", name->chars);
    return false;
  }

  ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
  pop();
  push(OBJ_VAL(bound));
  return true;
}

static ObjUpvalue *captureUpvalue(Value *local) {
  ObjUpvalue *prevUpvalue = NULL;
  ObjUpvalue *upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue *createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;

  if (prevUpvalue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

static void closeUpvalues(Value *last) {
  while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
    ObjUpvalue *upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static void defineMethod(ObjString *name) {
  Value method = peek(0);
  ObjClass *klass = AS_CLASS(peek(1));
  tableSet(&klass->methods, name, method);
  pop();
}

static bool isFalsey(Value value) {
  return IS_NONE(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
  ObjString *b = AS_STRING(peek(0));
  ObjString *a = AS_STRING(peek(1));

  int length = a->length + b->length;
  char *chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static void strMul() {
  int b = AS_INT(peek(0));
  ObjString *str = AS_STRING(peek(1));
  int length = str->length * b;
  char *chars = ALLOCATE(char, length + 1);
  for (char *ptr = chars; ptr < chars + length; ptr += str->length) {
    memcpy(ptr, str->chars, str->length);
  }
  chars[length] = '\0';

  ObjString *result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static void stringIndex(ObjString *str, int index) {
  if (index < 0)
    index = str->length + index;
  char value = str->chars[index];
  ObjString *result = copyString(&value, 1);
  push(OBJ_VAL(result));
}

static inline uint8_t read_byte(CallFrame *frame) { return *frame->ip++; }

static InterpretResult run() {
  CallFrame *frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (read_byte(frame))
#define READ_SHORT()                                                           \
  (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT()                                                        \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT()                                                   \
  (frame->closure->function->chunk.constants                                   \
       .values[READ_BYTE() << 16 | READ_BYTE() << 8 | READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("        ");
    for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[");
      printValue(*slot);
      printf("]");
    }
    printf("\n");
    disassembleInstruction(
        &frame->closure->function->chunk,
        (int)(frame->ip - frame->closure->function->chunk.code));
#endif // DEBUG_TRACE_EXECUTION

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_CALL: {
      int argCount = READ_BYTE();
      if (!callValue(peek(argCount), argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_LIST_BUILD: {
      ObjList *list = newList();
      uint8_t itemCount = READ_BYTE();

      push(OBJ_VAL(list));
      for (int i = itemCount; i > 0; i--) {
        appendToList(list, peek(i));
      }
      pop();

      while (itemCount-- > 0)
        pop();

      push(OBJ_VAL(list));
      break;
    }
    case OP_LIST_INDEX: {
      Value indexVal = pop();
      Value listVal = pop();

      if (!IS_INT(indexVal)) {
        runtimeError("Invalid index type.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int index = AS_INT(indexVal);

      if (!IS_LIST(listVal) && !IS_STRING(listVal)) {
        runtimeError("Invalid type to index into.");
        return INTERPRET_RUNTIME_ERROR;
      }

      if (IS_STRING(listVal)) {
        ObjString *str = AS_STRING(listVal);
        if (index < -str->length || index > str->length - 1) {
          runtimeError("String index out of range.");
          return INTERPRET_RUNTIME_ERROR;
        }
        stringIndex(str, index);
        break;
      }
      ObjList *list = AS_LIST(listVal);

      if (!isValidListIndex(list, index)) {
        runtimeError("List index out of range.");
        return INTERPRET_RUNTIME_ERROR;
      }

      push(getFromList(list, index));
      break;
    }
    case OP_LIST_STORE: {
      Value item = pop();
      Value indexVal = pop();
      Value listVal = pop();

      if (!IS_LIST(listVal)) {
        runtimeError("Cannot store value in a non-list.");
        return INTERPRET_RUNTIME_ERROR;
      }
      ObjList *list = AS_LIST(listVal);

      if (!IS_INT(indexVal)) {
        runtimeError("List index is not an integer.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int index = AS_INT(indexVal);

      if (!isValidListIndex(list, index)) {
        runtimeError("Invalid list index.");
        return INTERPRET_RUNTIME_ERROR;
      }

      setInList(list, index, item);
      push(item);
      break;
    }
    case OP_INHERIT: {
      Value superclass = peek(1);
      if (!IS_CLASS(superclass)) {
        runtimeError("Superclass must be a class.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjClass *subclass = AS_CLASS(peek(0));
      tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
      pop();
      break;
    }
    case OP_INVOKE: {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      if (!invoke(method, argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_CLASS: {
      push(OBJ_VAL(newClass(READ_STRING())));
      break;
    }
    case OP_CLOSURE: {
      ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure *closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++) {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal) {
          closure->upvalues[i] = captureUpvalue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      break;
    }
    case OP_CLOSE_UPVALUE: {
      closeUpvalues(vm.stackTop - 1);
      pop();
      break;
    }
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      push(constant);
      break;
    }
    case OP_CONSTANT_LONG: {
      Value constant = READ_LONG_CONSTANT();
      push(constant);
      break;
    }
    case OP_DEFINE_GLOBAL: {
      ObjString *name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      break;
    }
    case OP_NEGATE:
      if (IS_INT(peek(0))) {
        push(INT_VAL(-AS_INT(pop())));
      } else if (IS_NUMBER(peek(0))) {
        push(NUMBER_VAL(-AS_NUMBER(pop())));

      } else {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    case OP_INVERT: {
      if (!IS_INT(peek(0))) {
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(INT_VAL(~AS_INT(pop())));
      break;
    }
    case OP_NOT:
      push(BOOL_VAL(isFalsey(pop())));
      break;
    case OP_NONE:
      push(NONE_VAL);
      break;
    case OP_TRUE:
      push(BOOL_VAL(true));
      break;
    case OP_FALSE:
      push(BOOL_VAL(false));
      break;
    case OP_EQUAL: {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      break;
    }
    case OP_GET_GLOBAL: {
      ObjString *name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value)) {
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      break;
    }
    case OP_GET_SUPER: {
      ObjString *name = READ_STRING();
      ObjClass *superclass = AS_CLASS(pop());

      if (!bindMethod(superclass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_PROPERTY: {
      if (!IS_INSTANCE(peek(0))) {
        runtimeError("Only instances have properties.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(0));
      ObjString *name = READ_STRING();

      Value value;
      if (tableGet(&instance->fields, name, &value)) {
        pop();
        push(value);
        break;
      }

      if (!bindMethod(instance->klass, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      break;
    }
    case OP_SET_GLOBAL: {
      ObjString *name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SET_PROPERTY: {
      if (!IS_INSTANCE(peek(1))) {
        runtimeError("Only instances have fields.");
        return INTERPRET_RUNTIME_ERROR;
      }

      ObjInstance *instance = AS_INSTANCE(peek(1));
      tableSet(&instance->fields, READ_STRING(), peek(0));
      Value value = pop();
      pop();
      push(value);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      break;
    }
    case OP_SUPER_INVOKE: {
      ObjString *method = READ_STRING();
      int argCount = READ_BYTE();
      ObjClass *superclass = AS_CLASS(pop());

      if (!invokeFromClass(superclass, method, argCount)) {
        return INTERPRET_RUNTIME_ERROR;
      }

      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    case OP_GREATER: {
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
        runtimeError("Operands must be numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());
      push(BOOL_VAL(a > b));
      break;
    }
    case OP_LESS: {
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
        runtimeError("Operands must be numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double b = AS_NUMBER(pop());
      double a = AS_NUMBER(pop());
      push(BOOL_VAL(a < b));
      break;
    }
    case OP_ADD: {
      if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
        concatenate();
      } else if (IS_INT(peek(0)) && IS_INT(peek(1))) {
        int b = AS_INT(pop());
        int a = AS_INT(pop());
        push(INT_VAL(a + b));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a + b));
      } else {
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT: {
      if (IS_INT(peek(0)) && IS_INT(peek(1))) {
        int b = AS_INT(pop());
        int a = AS_INT(pop());
        push(INT_VAL(a - b));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a - b));
      } else {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_OR: {
      if (!IS_INT(peek(0)) || !IS_INT(peek(1))) {
        runtimeError("Operands must be two integers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int b = AS_INT(pop());
      int a = AS_INT(pop());
      push(INT_VAL(a | b));
      break;
    }
    case OP_XOR: {
      if (!IS_INT(peek(0)) || !IS_INT(peek(1))) {
        runtimeError("Operands must be two integers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int b = AS_INT(pop());
      int a = AS_INT(pop());
      push(INT_VAL(a ^ b));
      break;
    }
    case OP_AND: {
      if (!IS_INT(peek(0)) || !IS_INT(peek(1))) {
        runtimeError("Operands must be two integers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int b = AS_INT(pop());
      int a = AS_INT(pop());
      push(INT_VAL(a & b));
      break;
    }
    case OP_LSHIFT: {
      if (!IS_INT(peek(0)) || !IS_INT(peek(1))) {
        runtimeError("Operands must be two integers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int b = AS_INT(pop());
      int a = AS_INT(pop());
      push(INT_VAL(a << b));
      break;
    }
    case OP_RSHIFT: {
      if (!IS_INT(peek(0)) || !IS_INT(peek(1))) {
        runtimeError("Operands must be two integers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      int b = AS_INT(pop());
      int a = AS_INT(pop());
      push(INT_VAL(a >> b));
      break;
    }
    case OP_METHOD:
      defineMethod(READ_STRING());
      break;
    case OP_MULTIPLY: {
      if (IS_INT(peek(0)) && IS_STRING(peek(1))) {
        strMul();
      } else if (IS_INT(peek(0)) && IS_INT(peek(1))) {
        int b = AS_INT(pop());
        int a = AS_INT(pop());
        push(INT_VAL(a * b));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(a * b));
      } else {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_DIVIDE: {
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double b = AS_NUMBER(pop());
      if (b == 0) {
        runtimeError("Cannot divide by zero.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double a = AS_NUMBER(pop());
      push(NUMBER_VAL(a / b));
      break;
    }
    case OP_MODULUS: {
      if (IS_INT(peek(0)) && IS_INT(peek(1))) {
        int b = AS_INT(pop());
        if (b == 0) {
          runtimeError("Cannot divide by zero.");
          return INTERPRET_RUNTIME_ERROR;
        }
        int a = AS_INT(pop());
        push(INT_VAL(a % b));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        if (b == 0) {
          runtimeError("Cannot divide by zero.");
          return INTERPRET_RUNTIME_ERROR;
        }
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(fmod(a, b)));
      } else {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_FLOOR_DIVIDE: {
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double b = AS_NUMBER(pop());
      if (b == 0) {
        runtimeError("Cannot divide by zero.");
        return INTERPRET_RUNTIME_ERROR;
      }
      double a = AS_NUMBER(pop());
      push(INT_VAL((int)(a / b)));
      break;
    }
    case OP_EXPONENT: {
      if (IS_INT(peek(0)) && IS_INT(peek(1))) {
        int b = AS_INT(pop());
        int a = AS_INT(pop());
        push(INT_VAL(round(pow(a, b))));
      } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
        double b = AS_NUMBER(pop());
        double a = AS_NUMBER(pop());
        push(NUMBER_VAL(pow(a, b)));
      } else {
        runtimeError("Operands must be two numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_POP:
      pop();
      break;
    case OP_PRINT: {
      printValue(pop());
      printf("\n");
      break;
    }
    case OP_LOOP: {
      uint16_t offset = READ_SHORT();
      frame->ip -= offset;
      break;
    }
    case OP_JUMP: {
      uint16_t offset = READ_SHORT();
      frame->ip += offset;
      break;
    }
    case OP_JUMP_IF_FALSE: {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0)))
        frame->ip += offset;
      break;
    }
    case OP_RETURN: {
      Value result = pop();
      closeUpvalues(frame->slots);
      vm.frameCount--;
      if (vm.frameCount == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stackTop = frame->slots;
      push(result);
      frame = &vm.frames[vm.frameCount - 1];
      break;
    }
    default:
      UNREACHABLE();
      return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_LONG_CONSTANT
}

InterpretResult interpret(const char *source) {
  ObjFunction *function = compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure *closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}
