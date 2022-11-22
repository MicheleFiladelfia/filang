#include <string>
#include <fstream>
#include <sstream>
#include <sysexits.h>
#include <iostream>
#include "frontend/include/Lexer.hpp"

namespace Fi{
    //hidden(private) namespace
    namespace{
        std::string readSource(char* path){
            std::ifstream f(path);
            if(!f){
                std::cerr << "Cannot find " << path << ": No such file or directory"<<std::endl;
                std::exit(EX_NOINPUT);
            }
            std::stringstream buffer;
            buffer << f.rdbuf();
            return buffer.str(); 
        }

        void run(const std::string &source){
            Lexer lexer(source);

            std::vector<Token> tokens = lexer.scan();

            for(auto x : tokens){
                std::cout<<x.toString();
            }
        }
    }

    void runFile(char* path){
        run(readSource(path));
        if (errors.compiletime) std::exit(EX_DATAERR);
        if (errors.runtime) std::exit(EX_SOFTWARE);
    }


    void error(int line, std::string text){
        std::cerr<<"[line "<<line<<"] Error: "<<text<<std::endl;
        errors.compiletime = true;
    }
};

