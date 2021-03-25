#include "output.h"
#include "globals.h"

#include <string>

void write(std::ostream& os, Expression *expr);
void writeList(std::ostream& os, Expression *list) {
    Expression *car_obj = car(list);
    Expression *cdr_obj = cdr(list);

    write(os, car_obj);

    if (cdr_obj->exprType_ == Expression::LIST && cdr_obj != empty_list) {
        os << " ";
        writeList(os, cdr_obj);
    } else {
        if (cdr_obj == empty_list) { return; }
        os << " . ";
        write(os, cdr_obj);
    }
}

void write(std::ostream& os, Expression *expr) {
    int i=0;
    char c;
    std::string str;
    switch (expr->exprType_) {
    case Expression::LIST:
        os << "(";
        if (!isEmptyList(expr)) { writeList(os, expr); }
        os << ")";
        break;
    case Expression::ATOM:
        switch (expr->getAtom().atomType_) {
        case BOOL:
            os << std::boolalpha << expr->getAtom().getBool();
            break;
        case SYMBOL:
            os << expr->getAtom().getString();
            break;
        case NUM:
            os << expr->getAtom().getNumber();
            break;
        case PRIM_PROC:
            os << "#<primitiv_proc>";
            break;
        case COMP_PROC:
            os << "#<compound_proc>";
            break;
        case INPUT:
            os << "#<input_port>";
            break;
        case OUTPUT:
            os << "#<output_port>";
            break;
        case EOF_OBJECT:
            os << "#<eof>";
            break;
        case CHAR:
            c = expr->getAtom().getChar();
            os << "#\\";
            switch (c) {
                case '\n':
                    os << "newline";
                    break;
                case ' ':
                    os << "space";
                    break;
                default:
                    os << c;
            }
            break;
        case STR:
            str = expr->getAtom().getString();
            os << '"';
            while ((c = str[i++]) != '\0') {
                switch (c) {
                    case '\n':
                        os << "\\n";
                        break;
                    case '\\':
                        os << "\\\\";
                        break;
                    case '"':
                        os << "\\\"";
                        break;
                    default:
                        os << c;
                }
            }
            os << '"';
            break;
        default:
            std::cerr << "cannot write unknown type\n"; exit(1);
        }
        break;
    default:
        std::cerr << "illegal expression\n"; exit(1);
    }
}

std::ostream& operator<<(std::ostream& os, Expression* expr) {
    write(os, expr);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Expression& expr) {
    write(os, const_cast<Expression*>(&expr));
    return os;
}

