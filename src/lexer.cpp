#include "../inc/lexer.h"
#include <cctype>
#include <iostream>

Token::Token(TokenType t, std::string l, int line, int col)
    : type(t), lexeme(l), line(line), column(col) {}

std::string Token::toString() const
{
    static const char *typeNames[] = {
        "NUMBER", "FLOAT", "STRING", "IDENTIFIER",
        "INT8", "INT16", "INT32", "INT64",
        "UINT8", "UINT16", "UINT32", "UINT64",
        "FLOAT32", "FLOAT64",
        "BOOL", "VOID", "AUTO", "FILE",
        "FN", "RETURN", "CONST",
        "IF", "ELSE", "WHILE", "DO", "FOR", "SWITCH", "CASE", "DEFAULT",
        "BREAK", "CONTINUE",
        "CLASS", "PUBLIC", "PRIVATE", "PROTECTED",
        "VIRTUAL", "TEMPLATE", "TYPENAME",
        "TRY", "CATCH", "THROW",
        "LET",
        "TRUE", "FALSE",
        "NULLPTR",
        "PLUS", "MINUS", "STAR", "SLASH", "PERCENT",
        "PLUSPLUS", "MINUSMINUS",
        "EQ", "EQEQ", "NEQ",
        "LT", "GT", "LTE", "GTE",
        "AND", "OR", "NOT",
        "PLUSEQ", "MINUSEQ", "STAREQ", "SLASHEQ", "PERCENTEQ",
        "LPAREN", "RPAREN",
        "LBRACE", "RBRACE",
        "LBRACKET", "RBRACKET",
        "COMMA", "SEMICOLON",
        "COLON", "DOT",
        "ARROW",
        "EOF_TOKEN", "UNKNOWN"};
    return std::string(typeNames[(int)type]) + "('" + lexeme + "') at " +
           std::to_string(line) + ":" + std::to_string(column);
}

bool Token::isKeyword() const
{
    return (int)type >= (int)TokenType::INT8 &&
           (int)type <= (int)TokenType::NULLPTR;
}

bool Token::isType() const
{
    return (int)type >= (int)TokenType::INT8 &&
           (int)type <= (int)TokenType::FILE;
}

Lexer::Lexer(const std::string &src)
{
    reset(src);
}

void Lexer::reset(const std::string &src)
{
    source = src;
    position = 0;
    line = 1;
    column = 1;
    currentChar = source.empty() ? '\0' : source[0];
    initKeywords();
}

void Lexer::initKeywords()
{
    keywords["int8"] = TokenType::INT8;
    keywords["int16"] = TokenType::INT16;
    keywords["int32"] = TokenType::INT32;
    keywords["int64"] = TokenType::INT64;
    keywords["uint8"] = TokenType::UINT8;
    keywords["uint16"] = TokenType::UINT16;
    keywords["uint32"] = TokenType::UINT32;
    keywords["uint64"] = TokenType::UINT64;
    keywords["float32"] = TokenType::FLOAT32;
    keywords["float64"] = TokenType::FLOAT64;
    keywords["bool"] = TokenType::BOOL;
    keywords["void"] = TokenType::VOID;
    keywords["auto"] = TokenType::AUTO;
    keywords["file"] = TokenType::FILE;
    keywords["fn"] = TokenType::FN;
    keywords["return"] = TokenType::RETURN;
    keywords["const"] = TokenType::CONST;
    keywords["if"] = TokenType::IF;
    keywords["else"] = TokenType::ELSE;
    keywords["while"] = TokenType::WHILE;
    keywords["do"] = TokenType::DO;
    keywords["for"] = TokenType::FOR;
    keywords["switch"] = TokenType::SWITCH;
    keywords["case"] = TokenType::CASE;
    keywords["default"] = TokenType::DEFAULT;
    keywords["break"] = TokenType::BREAK;
    keywords["continue"] = TokenType::CONTINUE;
    keywords["class"] = TokenType::CLASS;
    keywords["public"] = TokenType::PUBLIC;
    keywords["private"] = TokenType::PRIVATE;
    keywords["protected"] = TokenType::PROTECTED;
    keywords["virtual"] = TokenType::VIRTUAL;
    keywords["template"] = TokenType::TEMPLATE;
    keywords["typename"] = TokenType::TYPENAME;
    keywords["try"] = TokenType::TRY;
    keywords["catch"] = TokenType::CATCH;
    keywords["throw"] = TokenType::THROW;
    keywords["let"] = TokenType::LET;
    keywords["true"] = TokenType::TRUE;
    keywords["false"] = TokenType::FALSE;
    keywords["nullptr"] = TokenType::NULLPTR;
    keywords["null"] = TokenType::NULLPTR;
}

void Lexer::advance()
{
    if (currentChar == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    position++;
    currentChar = (position < source.length()) ? source[position] : '\0';
}

char Lexer::peek() const
{
    return (position + 1 < source.length()) ? source[position + 1] : '\0';
}

void Lexer::skipWhitespace()
{
    while (currentChar == ' ' || currentChar == '\t' ||
           currentChar == '\n' || currentChar == '\r')
    {
        advance();
    }
}

void Lexer::skipLineComment()
{
    while (currentChar != '\n' && currentChar != '\0')
    {
        advance();
    }
}

void Lexer::skipBlockComment()
{
    advance();
    advance();
    while (!(currentChar == '*' && peek() == '/') && currentChar != '\0')
    {
        advance();
    }
    advance();
    advance();
}

void Lexer::skipComment()
{
    if (currentChar == '/' && peek() == '/')
    {
        skipLineComment();
    }
    else if (currentChar == '/' && peek() == '*')
    {
        skipBlockComment();
    }
}

Token Lexer::readNumber()
{
    std::string num;
    int startLine = line;
    int startCol = column;
    bool isFloat = false;

    while (std::isdigit(currentChar))
    {
        num += currentChar;
        advance();
    }

    if (currentChar == '.' && std::isdigit(peek()))
    {
        isFloat = true;
        num += currentChar;
        advance();
        while (std::isdigit(currentChar))
        {
            num += currentChar;
            advance();
        }
    }

    if (isFloat)
    {
        return Token(TokenType::FLOAT, num, startLine, startCol);
    }
    return Token(TokenType::NUMBER, num, startLine, startCol);
}

Token Lexer::readIdentifier()
{
    std::string ident;
    int startLine = line;
    int startCol = column;

    while (std::isalnum(currentChar) || currentChar == '_')
    {
        ident += currentChar;
        advance();
    }

    auto it = keywords.find(ident);
    if (it != keywords.end())
    {
        return Token(it->second, ident, startLine, startCol);
    }
    return Token(TokenType::IDENTIFIER, ident, startLine, startCol);
}

Token Lexer::readString()
{
    std::string str;
    int startLine = line;
    int startCol = column;

    advance();
    while (currentChar != '"' && currentChar != '\0')
    {
        if (currentChar == '\\')
        {
            str += currentChar;
            advance();
            if (currentChar == '\0')
                break;
            str += currentChar;
            advance();
        }
        else
        {
            str += currentChar;
            advance();
        }
    }
    if (currentChar == '"')
    {
        advance();
    }
    return Token(TokenType::STRING, str, startLine, startCol);
}

Token Lexer::readUnknown()
{
    std::string unk(1, currentChar);
    int startLine = line;
    int startCol = column;
    advance();
    return Token(TokenType::UNKNOWN, unk, startLine, startCol);
}

Token Lexer::nextToken()
{
    skipWhitespace();

    int startLine = line;
    int startCol = column;

    if (currentChar == '\0')
    {
        return Token(TokenType::EOF_TOKEN, "", startLine, startCol);
    }

    if (std::isdigit(currentChar))
    {
        return readNumber();
    }

    if (std::isalpha(currentChar) || currentChar == '_')
    {
        return readIdentifier();
    }

    if (currentChar == '"')
    {
        return readString();
    }

    if (currentChar == '/' && (peek() == '/' || peek() == '*'))
    {
        skipComment();
        return nextToken();
    }

    char c = currentChar;
    advance();

    switch (c)
    {
    case '+':
        if (currentChar == '+')
        {
            advance();
            return Token(TokenType::PLUSPLUS, "++", startLine, startCol);
        }
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::PLUSEQ, "+=", startLine, startCol);
        }
        return Token(TokenType::PLUS, "+", startLine, startCol);
    case '-':
        if (currentChar == '-')
        {
            advance();
            return Token(TokenType::MINUSMINUS, "--", startLine, startCol);
        }
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::MINUSEQ, "-=", startLine, startCol);
        }
        if (currentChar == '>')
        {
            advance();
            return Token(TokenType::ARROW, "->", startLine, startCol);
        }
        return Token(TokenType::MINUS, "-", startLine, startCol);
    case '*':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::STAREQ, "*=", startLine, startCol);
        }
        return Token(TokenType::STAR, "*", startLine, startCol);
    case '/':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::SLASHEQ, "/=", startLine, startCol);
        }
        return Token(TokenType::SLASH, "/", startLine, startCol);
    case '%':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::PERCENTEQ, "%=", startLine, startCol);
        }
        return Token(TokenType::PERCENT, "%", startLine, startCol);
    case '=':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::EQEQ, "==", startLine, startCol);
        }
        return Token(TokenType::EQ, "=", startLine, startCol);
    case '!':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::NEQ, "!=", startLine, startCol);
        }
        return Token(TokenType::NOT, "!", startLine, startCol);
    case '<':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::LTE, "<=", startLine, startCol);
        }
        return Token(TokenType::LT, "<", startLine, startCol);
    case '>':
        if (currentChar == '=')
        {
            advance();
            return Token(TokenType::GTE, ">=", startLine, startCol);
        }
        return Token(TokenType::GT, ">", startLine, startCol);
    case '&':
        if (currentChar == '&')
        {
            advance();
            return Token(TokenType::AND, "&&", startLine, startCol);
        }
        return Token(TokenType::UNKNOWN, "&", startLine, startCol);
    case '|':
        if (currentChar == '|')
        {
            advance();
            return Token(TokenType::OR, "||", startLine, startCol);
        }
        return Token(TokenType::UNKNOWN, "|", startLine, startCol);
    case '(':
        return Token(TokenType::LPAREN, "(", startLine, startCol);
    case ')':
        return Token(TokenType::RPAREN, ")", startLine, startCol);
    case '{':
        return Token(TokenType::LBRACE, "{", startLine, startCol);
    case '}':
        return Token(TokenType::RBRACE, "}", startLine, startCol);
    case '[':
        return Token(TokenType::LBRACKET, "[", startLine, startCol);
    case ']':
        return Token(TokenType::RBRACKET, "]", startLine, startCol);
    case ',':
        return Token(TokenType::COMMA, ",", startLine, startCol);
    case ';':
        return Token(TokenType::SEMICOLON, ";", startLine, startCol);
    case ':':
        return Token(TokenType::COLON, ":", startLine, startCol);
    case '.':
        return Token(TokenType::DOT, ".", startLine, startCol);
    default:
        return readUnknown();
    }
}

std::vector<Token> Lexer::tokenizeAll()
{
    std::vector<Token> tokens;
    Token token = nextToken();
    while (token.type != TokenType::EOF_TOKEN)
    {
        if (token.type != TokenType::UNKNOWN)
        {
            tokens.push_back(token);
        }
        else
        {
            std::cerr << "Unknown token: " << token.lexeme
                      << " at " << token.line << ":" << token.column << std::endl;
        }
        token = nextToken();
    }
    tokens.push_back(token);
    return tokens;
}