#ifndef filang
#define filang

#include <string>
#include <fstream>
#include <sstream>
#include <sysexits.h>
#include <iostream>
#include "frontend/include/Lexer.hpp"

namespace Fi{
    struct{
        bool compiletime = false;
        bool runtime = false;
    }errors;

    
    void runFile(char* path);
    void error(int line, std::string text);
}


#endif