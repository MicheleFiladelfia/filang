#include <iostream>
#include "Fi.hpp"

int main(int argc, char** argv){
    if(argc >= 2){
        Fi::runFile(argv[1]);
    }else{
        std::cerr<<"fatal error: no input files";
    }
    
    if(Fi::errors.compiletime) return 65;
    if(Fi::errors.runtime) return 70;
    return 0;
}