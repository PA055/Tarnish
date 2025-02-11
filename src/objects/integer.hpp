#pragma once

#include "number.hpp"

class Integer : public Number {
public:
    Integer(int value) : value(value) {}
    int value = 0;
};
