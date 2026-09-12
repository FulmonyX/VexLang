#ifndef VEXLANG_INC_CODEGEN_H
#define VEXLANG_INC_CODEGEN_H

#include "parser.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace vexlang
{

    class CodeGenerator
    {
    public:
        CodeGenerator();
        ~CodeGenerator();

        void generate(const Program &program);
        std::string getIR() const;

    private:
        std::unique_ptr<llvm::LLVMContext> context;
        std::unique_ptr<llvm::Module> module;
        std::unique_ptr<llvm::IRBuilder<>> builder;

        std::unordered_map<std::string, llvm::AllocaInst *> namedAllocas;

        llvm::Type *getLLVMType(const std::string &typeName);
        llvm::Value *generateExpr(ASTNode *node);
        llvm::Value *generateStmt(ASTNode *node);
        llvm::Value *generateBlock(BlockStmt *block);

        llvm::Value *generateNumber(NumberExpr *node);
        llvm::Value *generateVariable(VariableExpr *node);
        llvm::Value *generateBinary(BinaryExpr *node);
        llvm::Value *generateCall(CallExpr *node);

        llvm::Value *generateVariableDecl(VariableDecl *node);
        llvm::Value *generateAssignment(AssignmentStmt *node);
        llvm::Value *generateReturn(ReturnStmt *node);
        llvm::Value *generateIf(IfStmt *node);
        llvm::Value *generateWhile(WhileStmt *node);
        llvm::Value *generateGoto(GotoStmt *node);
        llvm::Value *generateLabel(LabelStmt *node);

        llvm::Function *generateFunction(FunctionDef *node);

        llvm::AllocaInst *createEntryBlockAlloca(llvm::Function *func,
                                                 const std::string &varName,
                                                 llvm::Type *type);
    };

} // namespace vexlang

#endif