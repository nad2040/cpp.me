#include "BufferReader.h"
#include <iostream>
#include <cassert>
#include <cstdlib>

extern Expression *empty_list;
extern Expression *_false;
extern Expression *_true;
extern Expression *quote_symbol;

void BufferReader::readBuffer() {
    std::string line;
    std::getline(std::cin, line);
    buffer_ += line;
}

void dumpBuffer(std::string& buf, int pos) {
    std::cout << "current position:" << pos << '\n';
    std::cout << "processed:" << buf.substr(0, pos) << '\n';
    std::cout << "unprocessed:" << buf.substr(pos) << '\n';
}

int listEnd(std::string& buffer, int pos) { //bug when string has ( or )
    assert(buffer[pos] == '(');
    int idx = pos+1;
    int count = 1;
    while (idx < buffer.size()) {
        if (buffer[idx] == '(') ++count;
        else if (buffer[idx] == ')') --count;
        if (count == 0) return ++idx;
        ++idx;
    }
    return pos;
}

int stringEnd(std::string& buffer, int pos) {
    assert(buffer[pos] == '"');
    int idx = pos+1;
    //do we need to handle \"
    while (idx < buffer.size()) {
        if (buffer[idx] == '"' && buffer[idx-1] != '\\') return ++idx;
        ++idx;
    }
    return pos;
}
namespace {
bool isDelimiter(char c) {
    return (isspace(c) || (c == '(') || (c == ')'));
}
}
int atomEnd(std::string& buffer, int pos) {
    assert(!isspace(buffer[pos]));
    int idx = pos+1;
    char c = buffer[idx];
    if (isspace(c) || c == ')') return pos; //without this line subtle bug
    while (idx < buffer.size()) { char c = buffer[idx]; if (isDelimiter(c)) break; ++idx; } //treat ` and "
    return idx-1;
}

Expression* BufferReader::readCdr() {
    static int count = 0;
    if (++count > 20) exit(1);
    dumpBuffer(buffer_, readPos_);
    int oldReadPos = readPos_;
    int index = readPos_;
    //first check empty list
    while(isspace(buffer_[index])) ++index;
    if (buffer_[index] == ')') {
        readPos_ = ++index;
        std::cout << "list reach end\n";
        return empty_list;
    }
    //read car
    readPos_ = index;
    Expression* first = nextExpression();
    if (!first) {
        readPos_ = oldReadPos;
        dumpBuffer(buffer_, readPos_);
        std::cout << "list car empty\n";
        return nullptr;
    }
    Expression* rest = readCdr();
    if (!rest) {
        readPos_ = oldReadPos; 
        dumpBuffer(buffer_, readPos_);
        std::cout << "list cdr empty\n";
        return nullptr;
    }
    return cons(first, rest);
}

Expression* BufferReader::readList() {
    std::cout << "start reading list\n";
    //parse (), (car . cdr), or list (car (cdr))
    assert(buffer_[readPos_] == '(');
    int oldReadPos_ = readPos_;
    int index = readPos_+1;
    while (index < buffer_.size() && isspace(buffer_[index])) ++index;
    if (buffer_[index] == ')') {
        readPos_ = ++index;
        std::cout << "return empty list\n";
        dumpBuffer(buffer_, readPos_);
        return empty_list;
    }
    readPos_ = index;
    Expression* first = nextExpression();
    if (!first) {
        readPos_ = oldReadPos_;
        return nullptr;
    }
    Expression* rest = readCdr();
    if (!rest) {
        readPos_ = oldReadPos_; 
        std::cout << "did not parse full list, revert readPos_\n";
        dumpBuffer(buffer_, readPos_);
        return nullptr;
    }
    std::cout << "list processed successfully, readPos_ " << readPos_ << '\n';
    dumpBuffer(buffer_, readPos_);
    return cons(first, rest);
}

Expression* BufferReader::readString() {
    assert(buffer_[readPos_] == '"');
    int stringBeg = readPos_;
    int stringEnd = ::stringEnd(buffer_, stringBeg);
    if (stringBeg != stringEnd) {
        readPos_ = stringEnd;
        return new Expression(Atom(buffer_.substr(stringBeg, stringEnd-stringBeg+1)));
    }
    return nullptr;
}

Expression* BufferReader::readQuotedExpression() {
    char c = buffer_[readPos_];
    assert(c == '\'');
    ++readPos_;
    Expression* expr = nextExpression();
    if (expr) return cons(quote_symbol, cons(expr, empty_list));
    else --readPos_; 
    return nullptr;
}

Expression* BufferReader::readNumber() {
    int atomBeg = readPos_;
    int atomEnd = ::atomEnd(buffer_, atomBeg);
    std::string token = buffer_.substr(atomBeg, atomEnd-atomBeg+1);
    readPos_ = atomEnd+1; //skipped validation of token as number
    std::cout << "number token:" << token << " size: " << token.size() << " beg: " << atomBeg << " end: " << atomEnd << " readPos_:" << readPos_ << '\n';
    return new Expression(Atom((long)atoi(token.c_str())));
}

/*
Expression* BufferReader::readCharacter() {
    char c = buffer_[readPos_];
    assert(c == '#');
    int atomBeg = readPos_;
    int atomEnd = ::atomEnd(buffer_, atomBeg);
    std::string token = buffer_.substr(atomBeg, atomEnd);
    if (token == "#\\space") {
        readPos_ = atomEnd;
        return new Expression(Atom(' '));
    } else if (token == "#\\newline") {
        readPos_ = atomEnd;
        return new Expression(Atom('\n'));
    }
    if (token.size() != 3) {
        std::cerr << "unexpexted character " << token << '\n';
        return nullptr;
    }
    readPos_ = atomEnd;
    return new Expression(Atom(token[2]));
}

Expression* BufferReader::readBoolean() {
    char c = buffer_[readPos_];
    assert(c == '#');
    char n = buffer_[readPos_+1];
    if (n == 't') { ++readPos_; return _true; }
    else if (n == 'f') { ++readPos_; return _false; }
    std::cerr << "unexpected #" << n << '\n';
    return nullptr;
}
*/

Expression* BufferReader::readHash() {
    char c = buffer_[readPos_];
    assert(c == '#');
    int atomBeg = readPos_;
    int atomEnd = ::atomEnd(buffer_, atomBeg);
    if (atomBeg != atomEnd) {
        std::string token = buffer_.substr(atomBeg, atomEnd-atomBeg+1);
        if (token == "#t") { readPos_ = atomEnd; return _true; }
        else if (token == "#f") { readPos_ = atomEnd; return _false; }
        else if (token == "#\\space") { readPos_ = atomEnd; return new Expression(Atom(' ')); }
        else if (token == "#\\newline") { readPos_ = atomEnd; return new Expression(Atom('\n')); }
        else if (token.size() == 3 && token[1] == '\\') { readPos_ = atomEnd; return new Expression(Atom(token[2])); }
        else {
            std::cerr << "discard unexpected " << token << '\n';
            readPos_ = atomEnd;
            return nullptr;
        }
    } else return nullptr;
}

Expression* BufferReader::readSymbol() {
    char c = buffer_[readPos_];
    assert(!isspace(c));
    int atomBeg = readPos_;
    int atomEnd = ::atomEnd(buffer_, atomBeg);
    //if (atomBeg != atomEnd) {
        readPos_ = atomEnd+1;
        std::string symbol = buffer_.substr(atomBeg, atomEnd-atomBeg+1);
        std::cout << "symbol:" << symbol << ":\n";
        return makeSymbol(buffer_.substr(atomBeg, atomEnd-atomBeg+1));
    //} else return nullptr;
}

//when fail should not change readPos_
Expression* BufferReader::nextExpression() {
    while (readPos_ < buffer_.size() && isspace(buffer_[readPos_])) ++readPos_;
    if (readPos_ == buffer_.size()) return nullptr; //at the end of buffer
    char c = buffer_[readPos_];
    char n = buffer_[readPos_+1]; //make sure within the bound
    if (c == '(') return readList();
    else if (c == '"') return readString();
    else if (c == '\'') return readQuotedExpression(); 
    else if ((c == '-' && isdigit(n) || isdigit(c))) return readNumber();
    else if (c == '#') return readHash();
    else return readSymbol();
    return nullptr;
}
