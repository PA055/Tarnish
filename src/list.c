#include "list.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include <stdbool.h>

void appendToList(ObjList *list, Value value) {
  if (list->capacity < list->count + 1) {
    int oldCapacity = list->capacity;
    list->capacity = GROW_CAPACITY(oldCapacity);
    list->items = GROW_ARRAY(Value, list->items, oldCapacity, list->capacity);
  }
  list->items[list->count] = value;
  list->count++;
}

void extendList(ObjList *list, ObjList *src) {
  for (int i = 0; i < src->count; i++) {
    appendToList(list, src->items[i]);
  }
}

void setInList(ObjList *list, int index, Value value) {
  if (index < 0)
    index = list->count + index;
  list->items[index] = value;
}

void setSliceInList(ObjList *list, ObjSlice *slice, ObjList *values) {
  int j = 0;
  for (int i = slice->start; i < slice->end; i += slice->skip) {
    setInList(list, i, values->items[j++]);
  }
}

Value getFromList(ObjList *list, int index) {
  if (index < 0)
    index = list->count + index;
  return list->items[index];
}

void deleteFromList(ObjList *list, int index) {
  if (index < 0)
    index = list->count + index;

  for (int i = index; i < list->count - 1; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->items[list->count - 1] = NONE_VAL;
  list->count--;
}

bool isValidListIndex(ObjList *list, int index) {
  if (index < -list->count || index > list->count - 1) {
    return false;
  }
  return true;
}

bool isValidListSlice(ObjList *list, ObjSlice *index) {
  return isValidListIndex(list, index->start) &&
         isValidListIndex(list, index->end);
}
