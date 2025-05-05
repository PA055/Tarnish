#ifndef tarnish_list_h
#define tarnish_list_h

#include "object.h"

void appendToList(ObjList* list, Value value);
void extendList(ObjList* list, ObjList* src);
void setInList(ObjList* list, int index, Value value);
void setSliceInList(ObjList* list, ObjSlice* slice, ObjList* values);
Value getFromList(ObjList* list, int index);
Value sliceFromList(ObjList* list, ObjSlice* index);
void deleteFromList(ObjList* list, int index);
bool isValidListIndex(ObjList* list, int index);
bool isValidListSlice(ObjList* list, ObjSlice* slice);

#endif // !tarnish_list_h
