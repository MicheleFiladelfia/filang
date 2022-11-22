#include "Token.hpp"
#include <iostream>

// A class to return values in the visitor pattern
template<typename VisitorImpl, typename VisitablePtr, typename ResultType>
class ValueGetter{
public:
    static ResultType GetValue(VisitablePtr& n){
        VisitorImpl vis;
        n.accept(vis);
        return vis.value;
    }

    void Return(ResultType value){
        this -> value = value;
    }
private:
    ResultType value;
};

// A base class, to allow downcasting
class Object{
protected:
    virtual void _f() { }
};

template<typename... Types>
class Visitor;

// single type Visitor  
template<typename T>
class Visitor<T> {
public:
    virtual void visit(T & visitable) = 0;
};

// multiple types visitor
template<typename T, typename... Types>
class Visitor<T, Types...> : public Visitor<Types...> {
public:
    using Visitor<Types...>::visit;

    virtual void visit(T & visitable) = 0;
};

template<typename... Types>
class Expr {
public:
    virtual void accept(Visitor<Types...>& visitor) = 0;
};

template<typename Derived, typename... Types>
class ExprImpl : public Expr<Types...> {
public:
    virtual void accept(Visitor<Types...>& visitor) {
        visitor.visit(static_cast<Derived&>(*this));
    }
};

class Binary;
class Literal;
class Logical;
class Grouping;

class Unary : public Object, public ExprImpl<Unary, Unary, Binary, Literal, Logical, Grouping> {};
class Binary : public Object, public ExprImpl<Binary, Unary, Binary, Literal, Logical, Grouping> {};
class Literal : public Object, public ExprImpl<Literal, Unary, Binary, Literal, Logical, Grouping> {};
class Logical: public Object, public ExprImpl<Logical, Unary, Binary, Literal, Logical, Grouping> {};
class Grouping:  public Object, public ExprImpl<Grouping, Unary, Binary, Literal, Logical, Grouping> {};

