#ifndef VEXLANG_INC_PARSER_H
#define VEXLANG_INC_PARSER_H

#include "lexer.h"
#include <vector>
#include <memory>
#include <string>

namespace vexlang
{

    class ASTNode
    {
    public:
        virtual ~ASTNode() = default;
    };

    class NumberExpr : public ASTNode
    {
    public:
        double value;
        explicit NumberExpr(double val);
    };

    class VariableExpr : public ASTNode
    {
    public:
        std::string name;
        explicit VariableExpr(const std::string &name);
    };

    class BinaryExpr : public ASTNode
    {
    public:
        TokenType op;
        std::unique_ptr<ASTNode> left;
        std::unique_ptr<ASTNode> right;
        BinaryExpr(TokenType op, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right);
    };

    class CallExpr : public ASTNode
    {
    public:
        std::string callee;
        std::vector<std::unique_ptr<ASTNode>> args;
        CallExpr(const std::string &callee, std::vector<std::unique_ptr<ASTNode>> args);
    };


    class VariableDecl : public ASTNode
    {
    public:
        std::string name;
        std::string type;
        std::unique_ptr<ASTNode> init;
        bool isConst;
        VariableDecl(const std::string &name, const std::string &type,
                     std::unique_ptr<ASTNode> init, bool isConst);
    };

    class AssignmentStmt : public ASTNode
    {
    public:
        std::string name;
        std::unique_ptr<ASTNode> value;
        AssignmentStmt(const std::string &name, std::unique_ptr<ASTNode> value);
    };

    class ReturnStmt : public ASTNode
    {
    public:
        std::unique_ptr<ASTNode> value;
        explicit ReturnStmt(std::unique_ptr<ASTNode> value);
    };

    class IfStmt : public ASTNode
    {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> thenBlock;
        std::unique_ptr<ASTNode> elseBlock;
        IfStmt(std::unique_ptr<ASTNode> condition,
               std::unique_ptr<ASTNode> thenBlock,
               std::unique_ptr<ASTNode> elseBlock);
    };

    class WhileStmt : public ASTNode
    {
    public:
        std::unique_ptr<ASTNode> condition;
        std::unique_ptr<ASTNode> body;
        WhileStmt(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body);
    };

    class BlockStmt : public ASTNode
    {
    public:
        std::vector<std::unique_ptr<ASTNode>> statements;
        explicit BlockStmt(std::vector<std::unique_ptr<ASTNode>> statements);
    };

    class GotoStmt : public ASTNode
    {
    public:
        std::string label;
        explicit GotoStmt(const std::string &label);
    };

    class LabelStmt : public ASTNode
    {
    public:
        std::string name;
        std::unique_ptr<ASTNode> stmt;
        LabelStmt(const std::string &name, std::unique_ptr<ASTNode> stmt);
    };

    class FunctionDef : public ASTNode
    {
    public:
        std::string name;
        std::string returnType;
        std::vector<std::string> params;
        std::vector<std::string> paramTypes;
        std::unique_ptr<ASTNode> body;
        FunctionDef(const std::string &name, const std::string &returnType,
                    std::vector<std::string> params, std::vector<std::string> paramTypes,
                    std::unique_ptr<ASTNode> body);
    };

    class Program : public ASTNode
    {
    public:
        std::vector<std::unique_ptr<FunctionDef>> functions;
        explicit Program(std::vector<std::unique_ptr<FunctionDef>> functions);
    };


    class Parser
    {
    public:
        explicit Parser(const std::vector<Token> &tokens);
        std::unique_ptr<Program> parse();

    private:
        std::vector<Token> tokens;
        size_t pos;
        int currentLine;
        int currentColumn;

        Token peek() const;
        Token peekAhead(int n) const;
        Token previous() const;
        Token advance();
        bool isAtEnd() const;
        bool check(TokenType type) const;
        bool match(TokenType type);
        Token expect(TokenType type, const std::string &errorMsg);
        void synchronize();

        std::unique_ptr<ASTNode> parseExpression();
        std::unique_ptr<ASTNode> parseAssignment();
        std::unique_ptr<ASTNode> parseLogicalOr();
        std::unique_ptr<ASTNode> parseLogicalAnd();
        std::unique_ptr<ASTNode> parseComparison();
        std::unique_ptr<ASTNode> parseAdditive();
        std::unique_ptr<ASTNode> parseMultiplicative();
        std::unique_ptr<ASTNode> parseUnary();
        std::unique_ptr<ASTNode> parsePrimary();
        std::unique_ptr<ASTNode> parseCall();

        std::unique_ptr<ASTNode> parseStatement();
        std::unique_ptr<ASTNode> parseVariableDecl();
        std::unique_ptr<ASTNode> parseAssignmentStmt();
        std::unique_ptr<ASTNode> parseReturnStmt();
        std::unique_ptr<ASTNode> parseIfStmt();
        std::unique_ptr<ASTNode> parseWhileStmt();
        std::unique_ptr<ASTNode> parseBlock();
        std::unique_ptr<ASTNode> parseGotoStmt();
        std::unique_ptr<ASTNode> parseLabelStmt();

        std::unique_ptr<FunctionDef> parseFunction();
        std::string parseType();
        std::vector<std::string> parseParamList();
        std::vector<std::string> parseParamTypeList();
    };

} // namespace vexlang

#endif