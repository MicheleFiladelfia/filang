#ifndef FILANG_SCANNER_H
#define FILANG_SCANNER_H

#include "token.h"

void init_scanner(const char *source);

Token scan_token();

#endif //FILANG_SCANNER_H
