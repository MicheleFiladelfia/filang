#ifndef FILANG_SCANNER_H
#define FILANG_SCANNER_H

#include "token.h"

void initScanner(const char *source);

Token scanToken();

#endif //FILANG_SCANNER_H
