#pragma once

#include "number.hpp"

class Float : public Number {
public:
    Float(double value) : value(value) {}
    double value = 0;
};
