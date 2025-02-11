#pragma once

#include <string>

void error(int line, std::string message);
void report(int line, std::string where, std::string message);
extern bool hadError;
