#include "astprinter.hpp"
#include "expr.hpp"

#include <memory>
#include <string>

std::string AstPrinter::print(std::shared_ptr<Expr<std::string>> expr) {
    return expr->accept(std::make_shared<AstPrinter>(*this));
}

std::string AstPrinter::visitLiteralExpr(std::shared_ptr<Literal<std::string>> expr) {
    return expr->value.__str__();
}

std::string AstPrinter::visitGroupingExpr(std::shared_ptr<Grouping<std::string>> expr) {
    return "( group " + expr->accept(std::make_shared<AstPrinter>(*this)) + " )";
}

std::string AstPrinter::visitUnaryExpr(std::shared_ptr<Unary<std::string>> expr) {
    return "(" + expr->operation.lexme + " " + expr->right->accept(std::make_shared<AstPrinter>(*this)) + ")";
}

std::string AstPrinter::visitBinaryExpr(std::shared_ptr<Binary<std::string>> expr) {
    return "(" + expr->left->accept(std::make_shared<AstPrinter>(*this)) + " " +
                 expr->operation.lexme + " " +
                 expr->right->accept(std::make_shared<AstPrinter>(*this)) + ")";
}
