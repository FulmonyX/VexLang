#ifndef VEXLANG_INC_LEXER_H
#define VEXLANG_INC_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>

namespace vexlang
{

    enum class TokenType
    {
        NUMBER,
        FLOAT,
        STRING,
        IDENTIFIER,
        INT8,
        INT16,
        INT32,
        INT64,
        UINT8,
        UINT16,
        UINT32,
        UINT64,
        FLOAT32,
        FLOAT64,
        BOOL,
        VOID,
        AUTO,
        FILE,
        FN,
        RETURN,
        CONST,
        IF,
        ELSE,
        WHILE,
        DO,
        FOR,
        SWITCH,
        CASE,
        DEFAULT,
        BREAK,
        CONTINUE,
        CLASS,
        PUBLIC,
        PRIVATE,
        PROTECTED,
        VIRTUAL,
        TEMPLATE,
        TYPENAME,
        TRY,
        CATCH,
        THROW,
        LET,
        TRUE,
        FALSE,
        NULLPTR,
        GOTO,
        PLUS,
        MINUS,
        STAR,
        SLASH,
        PERCENT,
        PLUSPLUS,
        MINUSMINUS,
        EQ,
        EQEQ,
        NEQ,
        LT,
        GT,
        LTE,
        GTE,
        AND,
        OR,
        NOT,
        PLUSEQ,
        MINUSEQ,
        STAREQ,
        SLASHEQ,
        PERCENTEQ,
        LPAREN,
        RPAREN,
        LBRACE,
        RBRACE,
        LBRACKET,
        RBRACKET,
        COMMA,
        SEMICOLON,
        COLON,
        DOT,
        ARROW,
        EOF_TOKEN,
        UNKNOWN
    };

    struct Token
    {
        TokenType type;
        std::string lexeme;
        int line;
        int column;

        Token(TokenType t = TokenType::UNKNOWN,
              std::string l = "",
              int line = 1,
              int col = 1);

        std::string toString() const;
        bool isKeyword() const;
        bool isType() const;
    };

    class Lexer
    {
    public:
        explicit Lexer(const std::string &src);
        Token nextToken();
        std::vector<Token> tokenizeAll();
        void reset(const std::string &src);

    private:
        std::string source;
        size_t position;
        int line;
        int column;
        char currentChar;
        std::unordered_map<std::string, TokenType> keywords;

        void initKeywords();
        void advance();
        char peek() const;
        void skipWhitespace();
        void skipLineComment();
        void skipBlockComment();
        void skipComment();
        Token readNumber();
        Token readIdentifier();
        Token readString();
        Token readUnknown();
        bool isHexDigit(char c) const;
        bool isOctDigit(char c) const;
        bool isBinDigit(char c) const;
    };

} // namespace vexlang

#endif