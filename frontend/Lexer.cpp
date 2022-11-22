#include "include/Lexer.hpp"

std::vector<Token> Lexer::scan(){
    while(!isEOF()){
        start = current;

        char c = peek();
        advance();

        switch (c){
            case '+':   addToken(TokenType::PLUS);        break; 
            case '-':   addToken(TokenType::MINUS);       break;
            case '*':   addToken(TokenType::STAR);        break;
            case '%':   addToken(TokenType::MODULO);      break;
            case ';':   addToken(TokenType::SEMICOLON);   break;
            case '~':   addToken(TokenType::BW_NOT);      break;
            case '&':   addToken(TokenType::BW_AND);      break;
            case '|':   addToken(TokenType::BW_OR);       break;
            case '^':   addToken(TokenType::BW_XOR);      break;
            case '(':   addToken(TokenType::LEFT_ROUND);  break;
            case ')':   addToken(TokenType::RIGHT_ROUND); break;
            case '{':   addToken(TokenType::RIGHT_BRACE); break;
            case '}':   addToken(TokenType::LEFT_BRACE);  break;
            case '\n':  line++;                           break;
            case ' ': case '\r': case '\t':               break;

            case '/':
                if(peek() == '/'){
                    advance();
                    while(peek() != '\n' && !isEOF())
                        advance();
                }else
                    addToken(TokenType::SLASH);
            break;
            case '=':
                if(peek() == '='){
                    advance();
                    addToken(TokenType::EQUAL_EQUAL);
                }else{
                    addToken(TokenType::EQUAL);
                }
            break;
            case '<':
                if(peek() == '='){
                    advance();
                    addToken(TokenType::LESS_EQUAL);
                }else if(peek() == '<'){
                    advance();
                    addToken(TokenType::BW_LSHIFT);
                }else{
                    addToken(TokenType::LESS);
                }
            break;
            case '>':
                if(peek() == '='){
                    advance();
                    addToken(TokenType::GREATER_EQUAL);
                }else if(peek() == '>'){
                    advance();
                    addToken(TokenType::BW_RSHIFT);
                }else{
                    addToken(TokenType::GREATER);
                }
            break;
            case '!':
                if(peek() == '='){
                    addToken(TokenType::BANG_EQUAL);
                    advance();
                }else{
                    addToken(TokenType::BANG);
                }
            break;
            default:
                if(isdigit(c)){
                    number();
                }else if(isalpha(c)){
                    identifier();
                }else{
                    Fi::error(line,"Unexpexted character: " + std::to_string(c));
                }
            break;
        }




    }

    tokens.push_back(*new Token(TokenType::END_OF_FILE,"",nullptr,line));
    return tokens;
}

void Lexer::number(){
    while(isdigit(peek())){
        advance();
    }

    if(peek() == '.' && isdigit(next())){
        advance();

        while(isdigit(peek())){
            advance();
        }
        

        addToken(TokenType::FLOAT,stod(source.substr(start, current - start)));
    }else{
        addToken(TokenType::INT, stoi(source.substr(start, current - start)));
    }

}

void Lexer::identifier(){
    while(isalpha(peek())){
        advance();
    }

    std::string lexeme = source.substr(start,current - start);

    if(identifiers.count(lexeme) > 0){
        addToken(identifiers[lexeme]);
    }else{
        addToken(TokenType::IDENTIFIER);
    }
}

bool Lexer::isEOF(){
    return current >= source.size();
}

void Lexer::advance(){
    if(current < source.size())
        current++;
}

char Lexer::peek(){
    if(!isEOF()){
        return source.at(current);
    }

    return '\0';
}

char Lexer::next(){
    if(current + 1 < source.size()){
        return source.at(current + 1);
    }

    return '\0';
}

void Lexer::addToken(TokenType type){
    addToken(type,nullptr);
}

void Lexer::addToken(TokenType type, std::any literal){
    std::cout<<"Lexeme:"<<start<<" "<<current<<std::endl;
    std::string lexeme = source.substr(start,current - start);
    tokens.push_back(*new Token(type,lexeme,literal,line));
}