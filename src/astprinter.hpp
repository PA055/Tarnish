#pragma once

#include <memory>
#include <string>

#include "expr.hpp"

class AstPrinter : public Visitor<std::string> {
public:
    std::string print(std::shared_ptr<Expr<std::string>> expr);

    std::string visitBinaryExpr(std::shared_ptr<Binary<std::string>> expr) override;
    std::string visitGroupingExpr(std::shared_ptr<Grouping<std::string>> expr) override;
    std::string visitLiteralExpr(std::shared_ptr<Literal<std::string>> expr) override;
    std::string visitUnaryExpr(std::shared_ptr<Unary<std::string>> expr) override;
};
