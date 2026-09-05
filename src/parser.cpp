#include "../inc/parser.h"
#include <iostream>
#include <stdexcept>

namespace vexlang
{
    NumberExpr::NumberExpr(double val) : value(val) {}

    VariableExpr::VariableExpr(const std::string &name) : name(name) {}

    BinaryExpr::BinaryExpr(TokenType op, std::unique_ptr<ASTNode> left, std::unique_ptr<ASTNode> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    CallExpr::CallExpr(const std::string &callee, std::vector<std::unique_ptr<ASTNode>> args)
        : callee(callee), args(std::move(args)) {}

    VariableDecl::VariableDecl(const std::string &name, const std::string &type,
                               std::unique_ptr<ASTNode> init, bool isConst)
        : name(name), type(type), init(std::move(init)), isConst(isConst) {}

    AssignmentStmt::AssignmentStmt(const std::string &name, std::unique_ptr<ASTNode> value)
        : name(name), value(std::move(value)) {}

    ReturnStmt::ReturnStmt(std::unique_ptr<ASTNode> value)
        : value(std::move(value)) {}

    IfStmt::IfStmt(std::unique_ptr<ASTNode> condition,
                   std::unique_ptr<ASTNode> thenBlock,
                   std::unique_ptr<ASTNode> elseBlock)
        : condition(std::move(condition)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}

    WhileStmt::WhileStmt(std::unique_ptr<ASTNode> condition, std::unique_ptr<ASTNode> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    BlockStmt::BlockStmt(std::vector<std::unique_ptr<ASTNode>> statements)
        : statements(std::move(statements)) {}

    FunctionDef::FunctionDef(const std::string &name, const std::string &returnType,
                             std::vector<std::string> params, std::vector<std::string> paramTypes,
                             std::unique_ptr<ASTNode> body)
        : name(name), returnType(returnType), params(std::move(params)),
          paramTypes(std::move(paramTypes)), body(std::move(body)) {}

    Program::Program(std::vector<std::unique_ptr<FunctionDef>> functions)
        : functions(std::move(functions)) {}

    Parser::Parser(const std::vector<Token> &tokens)
        : tokens(tokens), pos(0), currentLine(1), currentColumn(1) {}

    Token Parser::peek() const
    {
        if (pos < tokens.size())
            return tokens[pos];
        return Token(TokenType::EOF_TOKEN, "", currentLine, currentColumn);
    }

    Token Parser::peekAhead(int n) const
    {
        size_t idx = pos + n;
        if (idx < tokens.size())
            return tokens[idx];
        return Token(TokenType::EOF_TOKEN, "", currentLine, currentColumn);
    }

    Token Parser::previous() const
    {
        if (pos > 0)
            return tokens[pos - 1];
        return Token(TokenType::EOF_TOKEN, "", currentLine, currentColumn);
    }

    Token Parser::advance()
    {
        if (!isAtEnd())
        {
            Token t = tokens[pos];
            pos++;
            currentLine = t.line;
            currentColumn = t.column;
            return t;
        }
        return Token(TokenType::EOF_TOKEN, "", currentLine, currentColumn);
    }

    bool Parser::isAtEnd() const
    {
        return pos >= tokens.size() || tokens[pos].type == TokenType::EOF_TOKEN;
    }

    bool Parser::check(TokenType type) const
    {
        if (isAtEnd())
            return false;
        return peek().type == type;
    }

    bool Parser::match(TokenType type)
    {
        if (check(type))
        {
            advance();
            return true;
        }
        return false;
    }

    Token Parser::expect(TokenType type, const std::string &errorMsg)
    {
        if (check(type))
        {
            return advance();
        }
        throw std::runtime_error(errorMsg + " at " +
                                 std::to_string(peek().line) + ":" +
                                 std::to_string(peek().column));
    }

    void Parser::synchronize()
    {
        while (!isAtEnd())
        {
            if (previous().type == TokenType::SEMICOLON)
                return;
            switch (peek().type)
            {
            case TokenType::FN:
            case TokenType::CLASS:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::RETURN:
                return;
            default:
                advance();
            }
        }
    }

    std::string Parser::parseType()
    {
        if (check(TokenType::INT8) || check(TokenType::INT16) ||
            check(TokenType::INT32) || check(TokenType::INT64) ||
            check(TokenType::UINT8) || check(TokenType::UINT16) ||
            check(TokenType::UINT32) || check(TokenType::UINT64) ||
            check(TokenType::FLOAT32) || check(TokenType::FLOAT64) ||
            check(TokenType::BOOL) || check(TokenType::VOID) ||
            check(TokenType::AUTO) || check(TokenType::FILE))
        {
            Token t = advance();
            return t.lexeme;
        }
        if (check(TokenType::IDENTIFIER))
        {
            Token t = advance();
            return t.lexeme;
        }
        return "";
    }

    std::unique_ptr<ASTNode> Parser::parseExpression()
    {
        return parseAssignment();
    }

    std::unique_ptr<ASTNode> Parser::parseAssignment()
    {
        auto left = parseLogicalOr();
        if (match(TokenType::EQ))
        {
            auto right = parseAssignment();
            if (auto *var = dynamic_cast<VariableExpr *>(left.get()))
            {
                return std::make_unique<AssignmentStmt>(var->name, std::move(right));
            }
            throw std::runtime_error("Invalid assignment target");
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseLogicalOr()
    {
        auto left = parseLogicalAnd();
        while (match(TokenType::OR))
        {
            auto right = parseLogicalAnd();
            left = std::make_unique<BinaryExpr>(TokenType::OR, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseLogicalAnd()
    {
        auto left = parseComparison();
        while (match(TokenType::AND))
        {
            auto right = parseComparison();
            left = std::make_unique<BinaryExpr>(TokenType::AND, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseComparison()
    {
        auto left = parseAdditive();
        while (match(TokenType::LT) || match(TokenType::GT) ||
               match(TokenType::LTE) || match(TokenType::GTE) ||
               match(TokenType::EQEQ) || match(TokenType::NEQ))
        {
            TokenType op = previous().type;
            auto right = parseAdditive();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseAdditive()
    {
        auto left = parseMultiplicative();
        while (match(TokenType::PLUS) || match(TokenType::MINUS))
        {
            TokenType op = previous().type;
            auto right = parseMultiplicative();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseMultiplicative()
    {
        auto left = parseUnary();
        while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT))
        {
            TokenType op = previous().type;
            auto right = parseUnary();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    std::unique_ptr<ASTNode> Parser::parseUnary()
    {
        if (match(TokenType::MINUS))
        {
            auto operand = parseUnary();
            return std::make_unique<BinaryExpr>(TokenType::MINUS,
                                                std::make_unique<NumberExpr>(0),
                                                std::move(operand));
        }
        if (match(TokenType::NOT))
        {
            auto operand = parseUnary();
            return std::make_unique<BinaryExpr>(TokenType::NOT,
                                                std::make_unique<NumberExpr>(0),
                                                std::move(operand));
        }
        return parsePrimary();
    }

    std::unique_ptr<ASTNode> Parser::parsePrimary()
    {
        if (match(TokenType::NUMBER))
        {
            return std::make_unique<NumberExpr>(std::stod(previous().lexeme));
        }
        if (match(TokenType::FLOAT))
        {
            return std::make_unique<NumberExpr>(std::stod(previous().lexeme));
        }
        if (match(TokenType::TRUE))
        {
            return std::make_unique<NumberExpr>(1);
        }
        if (match(TokenType::FALSE))
        {
            return std::make_unique<NumberExpr>(0);
        }
        if (match(TokenType::IDENTIFIER))
        {
            std::string name = previous().lexeme;
            if (check(TokenType::LPAREN))
            {
                return parseCall();
            }
            return std::make_unique<VariableExpr>(name);
        }
        if (match(TokenType::LPAREN))
        {
            auto expr = parseExpression();
            expect(TokenType::RPAREN, "Expected ')'");
            return expr;
        }
        if (match(TokenType::STRING))
        {
            return std::make_unique<VariableExpr>(previous().lexeme);
        }
        throw std::runtime_error("Unexpected token: " + peek().lexeme);
    }

    std::unique_ptr<ASTNode> Parser::parseCall()
    {
        std::string callee = previous().lexeme;
        expect(TokenType::LPAREN, "Expected '('");
        std::vector<std::unique_ptr<ASTNode>> args;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                args.push_back(parseExpression());
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::RPAREN, "Expected ')'");
        return std::make_unique<CallExpr>(callee, std::move(args));
    }


    std::unique_ptr<ASTNode> Parser::parseStatement()
    {
        if (check(TokenType::LET))
        {
            return parseVariableDecl();
        }
        if (check(TokenType::IF))
        {
            return parseIfStmt();
        }
        if (check(TokenType::WHILE))
        {
            return parseWhileStmt();
        }
        if (check(TokenType::RETURN))
        {
            return parseReturnStmt();
        }
        if (check(TokenType::LBRACE))
        {
            return parseBlock();
        }
        if (check(TokenType::SEMICOLON))
        {
            advance();
            return nullptr;
        }
        auto expr = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';'");
        return expr;
    }

    std::unique_ptr<ASTNode> Parser::parseVariableDecl()
    {
        expect(TokenType::LET, "Expected 'let'");
        Token name = expect(TokenType::IDENTIFIER, "Expected variable name");
        std::string type = "";
        if (match(TokenType::COLON))
        {
            type = parseType();
        }
        std::unique_ptr<ASTNode> init = nullptr;
        if (match(TokenType::EQ))
        {
            init = parseExpression();
        }
        bool isConst = false;
        if (match(TokenType::CONST))
        {
            isConst = true;
        }
        expect(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<VariableDecl>(name.lexeme, type, std::move(init), isConst);
    }

    std::unique_ptr<ASTNode> Parser::parseAssignmentStmt()
    {
        Token name = expect(TokenType::IDENTIFIER, "Expected variable name");
        expect(TokenType::EQ, "Expected '='");
        auto value = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<AssignmentStmt>(name.lexeme, std::move(value));
    }

    std::unique_ptr<ASTNode> Parser::parseReturnStmt()
    {
        expect(TokenType::RETURN, "Expected 'return'");
        std::unique_ptr<ASTNode> value = nullptr;
        if (!check(TokenType::SEMICOLON))
        {
            value = parseExpression();
        }
        expect(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<ReturnStmt>(std::move(value));
    }

    std::unique_ptr<ASTNode> Parser::parseIfStmt()
    {
        expect(TokenType::IF, "Expected 'if'");
        expect(TokenType::LPAREN, "Expected '('");
        auto condition = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        auto thenBlock = parseBlock();
        std::unique_ptr<ASTNode> elseBlock = nullptr;
        if (match(TokenType::ELSE))
        {
            elseBlock = parseBlock();
        }
        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
    }

    std::unique_ptr<ASTNode> Parser::parseWhileStmt()
    {
        expect(TokenType::WHILE, "Expected 'while'");
        expect(TokenType::LPAREN, "Expected '('");
        auto condition = parseExpression();
        expect(TokenType::RPAREN, "Expected ')'");
        auto body = parseBlock();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ASTNode> Parser::parseBlock()
    {
        expect(TokenType::LBRACE, "Expected '{'");
        std::vector<std::unique_ptr<ASTNode>> statements;
        while (!check(TokenType::RBRACE) && !isAtEnd())
        {
            auto stmt = parseStatement();
            if (stmt)
            {
                statements.push_back(std::move(stmt));
            }
        }
        expect(TokenType::RBRACE, "Expected '}'");
        return std::make_unique<BlockStmt>(std::move(statements));
    }


    std::vector<std::string> Parser::parseParamList()
    {
        std::vector<std::string> params;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                Token param = expect(TokenType::IDENTIFIER, "Expected parameter name");
                params.push_back(param.lexeme);
            } while (match(TokenType::COMMA));
        }
        return params;
    }

    std::vector<std::string> Parser::parseParamTypeList()
    {
        std::vector<std::string> paramTypes;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                std::string type = parseType();
                paramTypes.push_back(type);
                expect(TokenType::IDENTIFIER, "Expected parameter name");
            } while (match(TokenType::COMMA));
        }
        return paramTypes;
    }

    std::unique_ptr<FunctionDef> Parser::parseFunction()
    {
        expect(TokenType::FN, "Expected 'fn'");
        Token name = expect(TokenType::IDENTIFIER, "Expected function name");
        expect(TokenType::LPAREN, "Expected '('");
        auto params = parseParamList();
        expect(TokenType::RPAREN, "Expected ')'");
        std::string returnType = "void";
        if (match(TokenType::ARROW))
        {
            returnType = parseType();
        }
        auto body = parseBlock();
        return std::make_unique<FunctionDef>(name.lexeme, returnType,
                                             std::move(params), std::move(params),
                                             std::move(body));
    }

    std::unique_ptr<Program> Parser::parse()
    {
        std::vector<std::unique_ptr<FunctionDef>> functions;
        while (!isAtEnd())
        {
            if (check(TokenType::FN))
            {
                functions.push_back(parseFunction());
            }
            else
            {
                throw std::runtime_error("Expected function definition at " +
                                         std::to_string(peek().line) + ":" +
                                         std::to_string(peek().column));
            }
        }
        return std::make_unique<Program>(std::move(functions));
    }

} // namespace vexlang