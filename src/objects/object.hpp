#pragma once

#include <ostream>

class Object {
public:
    friend std::ostream& operator<<(std::ostream& os, const Object& obj) {
        os << obj.__str__();
        return os;
    }

    virtual std::string __str__() const = 0;
};
