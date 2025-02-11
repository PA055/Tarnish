#pragma once

#include "object.hpp"
#include <string>

class String : public Object {
public:
    String(std::string value) : value(value) {}
    std::string value;
    std::string __str__() const {
        return value;
    }
};
