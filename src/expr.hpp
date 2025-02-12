#pragma once

#include <memory>

#include "token.hpp"

template<class R>
class Binary;


template<class R>
class Grouping;


template<class R>
class Literal;


template<class R>
class Unary;


template <class R>
class Visitor {
public:
    virtual ~Visitor() = default;
	virtual R visitBinaryExpr(std::shared_ptr<Binary<R>> expr) = 0;
	virtual R visitGroupingExpr(std::shared_ptr<Grouping<R>> expr) = 0;
	virtual R visitLiteralExpr(std::shared_ptr<Literal<R>> expr) = 0;
	virtual R visitUnaryExpr(std::shared_ptr<Unary<R>> expr) = 0;
};

template <class R>
class Expr {
public:
    virtual ~Expr() = default;
    virtual R accept(std::shared_ptr<Visitor<R>> visitor) = 0;
};

template <class R>
class Binary :
        public Expr<R>,
        public std::enable_shared_from_this<Binary<R>> {
public:
    Binary(std::shared_ptr<Expr<R>> left, Token operation, std::shared_ptr<Expr<R>> right) :
			left(left),
			operation(operation),
			right(right) {}

	R accept(std::shared_ptr<Visitor<R>> visitor) override {
        return visitor->visitBinaryExpr(this->shared_from_this());
    }
	std::shared_ptr<Expr<R>> left;
	Token operation;
	std::shared_ptr<Expr<R>> right;
};

template <class R>
class Grouping :
        public Expr<R>,
        public std::enable_shared_from_this<Grouping<R>> {
public:
    Grouping(std::shared_ptr<Expr<R>> expression) :
			expression(expression) {}

	R accept(std::shared_ptr<Visitor<R>> visitor) override {
        return visitor->visitGroupingExpr(this->shared_from_this());
    }
	std::shared_ptr<Expr<R>> expression;
};

template <class R>
class Literal :
        public Expr<R>,
        public std::enable_shared_from_this<Literal<R>> {
public:
    Literal(Object value) :
			value(value) {}

	R accept(std::shared_ptr<Visitor<R>> visitor) override {
        return visitor->visitLiteralExpr(this->shared_from_this());
    }
	Object value;
};

template <class R>
class Unary :
        public Expr<R>,
        public std::enable_shared_from_this<Unary<R>> {
public:
    Unary(Token operation, std::shared_ptr<Expr<R>> right) :
			operation(operation),
			right(right) {}

	R accept(std::shared_ptr<Visitor<R>> visitor) override {
        return visitor->visitUnaryExpr(this->shared_from_this());
    }
	Token operation;
	std::shared_ptr<Expr<R>> right;
};
