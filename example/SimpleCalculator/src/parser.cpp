#include "./parser.h"
#include "./token.h"
using namespace std;


#define NI(t) CharID<NonTerm##t>()
#define TI(t) CharID<Token##t>()

#define add_binary_rule(op, assoc) \
parser( NI(Expr), \
        { NI(Expr), TI(op), NI(Expr) }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 3); \
            auto left  = dynamic_pointer_cast<NonTermExpr>(ts[0]); \
            auto right = dynamic_pointer_cast<NonTermExpr>(ts[2]); \
            assert(left && right); \
            auto astleft = dynamic_pointer_cast<ASTNodeExpr>(left->astnode); \
            auto astright = dynamic_pointer_cast<ASTNodeExpr>(right->astnode); \
            auto ast = make_shared<BinaryOperatorExpr>( \
                    &c, \
                    BinaryOperatorExpr::BinaryOperatorType::op, \
                    astleft, \
                    astright); \
            return make_shared<NonTermExpr>(ast); \
        }, assoc );

#define add_unary_rule(t1, t2, isleft, type, assoc) \
parser( NI(Expr), \
        { t1, t2 }, \
        [](auto c, auto ts) { \
            assert(ts.size() == 2); \
            auto expr = dynamic_pointer_cast<NonTermExpr>(ts[isleft ? 0 : 1]); \
            assert(expr); \
            auto astexpr = dynamic_pointer_cast<ASTNodeExpr>(expr->astnode); \
            auto ast = make_shared<UnaryOperatorExpr>( \
                    &c, \
                    UnaryOperatorExpr::UnaryOperatorType::type, \
                    astexpr); \
            return make_shared<NonTermExpr>(ast); \
        }, assoc );


CalcParser::CalcParser()
{
    DCParser& parser = *this;

    add_unary_rule( NI(Expr), TI(PLUSPLUS),   true, POS_INC, RuleAssocitiveLeft );
    add_unary_rule( NI(Expr), TI(MINUSMINUS), true, POS_DEC, RuleAssocitiveLeft );

    parser.__________();

    add_unary_rule( TI(PLUSPLUS),   NI(Expr), false, PRE_INC, RuleAssocitiveRight );
    add_unary_rule( TI(MINUSMINUS), NI(Expr), false, PRE_DEC, RuleAssocitiveRight );

    parser.__________();

    add_binary_rule(CARET, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(MULTIPLY, RuleAssocitiveLeft);
    add_binary_rule(DIVISION, RuleAssocitiveLeft);
    add_binary_rule(REMAINDER, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(PLUS, RuleAssocitiveLeft);
    add_binary_rule(MINUS, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(GREATERTHAN, RuleAssocitiveLeft);
    add_binary_rule(LESSTHAN, RuleAssocitiveLeft);
    add_binary_rule(GREATEREQUAL, RuleAssocitiveLeft);
    add_binary_rule(LESSEQUAL, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(EQUAL, RuleAssocitiveLeft);
    add_binary_rule(NOTEQUAL, RuleAssocitiveLeft);

    parser.__________();

    add_binary_rule(ASSIGNMENT, RuleAssocitiveRight);


    parser.generate_table();
}
