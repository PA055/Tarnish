#include "token.hpp"
#include <ostream>
#include <string>

std::string Object::__str__() const {
    if (isString()) return std::get<std::string>(mData);
    else if (isDouble()) return std::to_string(std::get<double>(mData));
    else if (isInteger()) return std::to_string(std::get<int>(mData));
    else if (isBool()) return std::get<bool>(mData) ? "true" : "false";
    else if (isNone()) return "none";
    else return "Unknown Type";
}

bool Object::isString() const {
    return std::holds_alternative<std::string>(mData);
}

bool Object::isDouble() const {
    return std::holds_alternative<double>(mData);
}

bool Object::isInteger() const {
    return std::holds_alternative<int>(mData);
}

bool Object::isBool() const {
    return std::holds_alternative<bool>(mData);
}

bool Object::isNone() const {
    return std::holds_alternative<None>(mData);
}

bool operator==(const Object& lhs, const Object& rhs) {
    return lhs.mData == rhs.mData;
}

std::ostream& operator<<(std::ostream& os, const Object& obj) {
    os << obj.__str__();
    return os;
}
