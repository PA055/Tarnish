#pragma once

#include "number.hpp"
#include <string>

class Integer : public Number {
public:
    Integer(int value) : value(value) {}
    int value = 0;
    std::string __str__() const {
        return std::to_string(value);
    }
};
