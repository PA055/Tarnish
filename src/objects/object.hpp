#pragma once

#include <ostream>

class Object {
public:
    friend std::ostream& operator<<(std::ostream& os, const Object& obj) {
        return os;
    }
};
