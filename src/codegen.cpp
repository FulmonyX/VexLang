#include "../inc/codegen.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <stdexcept>

namespace vexlang
{

    CodeGenerator::CodeGenerator()
    {
        context = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("VexLang", *context);
        builder = std::make_unique<llvm::IRBuilder<>>(*context);
    }

    CodeGenerator::~CodeGenerator() = default;

    llvm::Type *CodeGenerator::getLLVMType(const std::string &typeName)
    {
        if (typeName == "int8" || typeName == "uint8")
            return builder->getInt8Ty();
        if (typeName == "int16" || typeName == "uint16")
            return builder->getInt16Ty();
        if (typeName == "int32" || typeName == "uint32")
            return builder->getInt32Ty();
        if (typeName == "int64" || typeName == "uint64")
            return builder->getInt64Ty();
        if (typeName == "float32")
            return builder->getFloatTy();
        if (typeName == "float64")
            return builder->getDoubleTy();
        if (typeName == "bool")
            return builder->getInt1Ty();
        if (typeName == "void")
            return builder->getVoidTy();
        return builder->getInt32Ty();
    }

    llvm::AllocaInst *CodeGenerator::createEntryBlockAlloca(llvm::Function *func,
                                                            const std::string &varName,
                                                            llvm::Type *type)
    {
        llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                                     func->getEntryBlock().begin());
        return tmpBuilder.CreateAlloca(type, nullptr, varName);
    }

    llvm::Value *CodeGenerator::generateExpr(ASTNode *node)
    {
        if (auto *n = dynamic_cast<NumberExpr *>(node))
            return generateNumber(n);
        if (auto *n = dynamic_cast<VariableExpr *>(node))
            return generateVariable(n);
        if (auto *n = dynamic_cast<BinaryExpr *>(node))
            return generateBinary(n);
        if (auto *n = dynamic_cast<CallExpr *>(node))
            return generateCall(n);
        throw std::runtime_error("Unknown expression node");
    }

    llvm::Value *CodeGenerator::generateStmt(ASTNode *node)
    {
        if (!node)
            return nullptr;
        if (auto *n = dynamic_cast<VariableDecl *>(node))
            return generateVariableDecl(n);
        if (auto *n = dynamic_cast<AssignmentStmt *>(node))
            return generateAssignment(n);
        if (auto *n = dynamic_cast<ReturnStmt *>(node))
            return generateReturn(n);
        if (auto *n = dynamic_cast<IfStmt *>(node))
            return generateIf(n);
        if (auto *n = dynamic_cast<WhileStmt *>(node))
            return generateWhile(n);
        if (auto *n = dynamic_cast<BlockStmt *>(node))
            return generateBlock(n);
        if (auto *n = dynamic_cast<GotoStmt *>(node))
            return generateGoto(n);
        if (auto *n = dynamic_cast<LabelStmt *>(node))
            return generateLabel(n);
        return generateExpr(node);
    }

    llvm::Value *CodeGenerator::generateBlock(BlockStmt *block)
    {
        if (!block)
            return nullptr;
        llvm::Value *last = nullptr;
        for (auto &stmt : block->statements)
        {
            last = generateStmt(stmt.get());
        }
        return last;
    }

    llvm::Value *CodeGenerator::generateNumber(NumberExpr *node)
    {
        return llvm::ConstantInt::get(builder->getInt32Ty(),
                                      static_cast<int>(node->value));
    }

    llvm::Value *CodeGenerator::generateVariable(VariableExpr *node)
    {
        auto it = namedAllocas.find(node->name);
        if (it == namedAllocas.end())
        {
            throw std::runtime_error("Unknown variable: " + node->name);
        }
        return builder->CreateLoad(builder->getInt32Ty(), it->second, node->name);
    }

    llvm::Value *CodeGenerator::generateBinary(BinaryExpr *node)
    {
        llvm::Value *L = generateExpr(node->left.get());
        llvm::Value *R = generateExpr(node->right.get());

        switch (node->op)
        {
        case TokenType::PLUS:
            return builder->CreateAdd(L, R, "addtmp");
        case TokenType::MINUS:
            return builder->CreateSub(L, R, "subtmp");
        case TokenType::STAR:
            return builder->CreateMul(L, R, "multmp");
        case TokenType::SLASH:
            return builder->CreateSDiv(L, R, "divtmp");
        case TokenType::PERCENT:
            return builder->CreateSRem(L, R, "modtmp");
        case TokenType::EQEQ:
            return builder->CreateICmpEQ(L, R, "eqtmp");
        case TokenType::NEQ:
            return builder->CreateICmpNE(L, R, "netmp");
        case TokenType::LT:
            return builder->CreateICmpSLT(L, R, "lttmp");
        case TokenType::GT:
            return builder->CreateICmpSGT(L, R, "gttmp");
        case TokenType::LTE:
            return builder->CreateICmpSLE(L, R, "letmp");
        case TokenType::GTE:
            return builder->CreateICmpSGE(L, R, "getmp");
        case TokenType::AND:
            return builder->CreateAnd(L, R, "andtmp");
        case TokenType::OR:
            return builder->CreateOr(L, R, "ortmp");
        default:
            throw std::runtime_error("Unknown binary operator");
        }
    }

    llvm::Value *CodeGenerator::generateCall(CallExpr *node)
    {
        llvm::Function *callee = module->getFunction(node->callee);
        if (!callee)
        {
            throw std::runtime_error("Unknown function: " + node->callee);
        }

        std::vector<llvm::Value *> args;
        for (auto &arg : node->args)
        {
            args.push_back(generateExpr(arg.get()));
        }

        return builder->CreateCall(callee, args, "calltmp");
    }

    llvm::Value *CodeGenerator::generateVariableDecl(VariableDecl *node)
    {
        llvm::Function *func = builder->GetInsertBlock()->getParent();
        llvm::Type *type = builder->getInt32Ty();

        llvm::AllocaInst *alloca = createEntryBlockAlloca(func, node->name, type);
        namedAllocas[node->name] = alloca;

        if (node->init)
        {
            llvm::Value *initVal = generateExpr(node->init.get());
            builder->CreateStore(initVal, alloca);
        }

        return alloca;
    }

    llvm::Value *CodeGenerator::generateAssignment(AssignmentStmt *node)
    {
        auto it = namedAllocas.find(node->name);
        if (it == namedAllocas.end())
        {
            throw std::runtime_error("Unknown variable: " + node->name);
        }

        llvm::Value *value = generateExpr(node->value.get());
        return builder->CreateStore(value, it->second);
    }

    llvm::Value *CodeGenerator::generateReturn(ReturnStmt *node)
    {
        if (node->value)
        {
            llvm::Value *retVal = generateExpr(node->value.get());
            return builder->CreateRet(retVal);
        }
        return builder->CreateRetVoid();
    }

    llvm::Value *CodeGenerator::generateIf(IfStmt *node)
    {
        llvm::Value *cond = generateExpr(node->condition.get());

        llvm::Function *func = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(*context, "then", func);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(*context, "else");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(*context, "ifcont");

        builder->CreateCondBr(cond, thenBB, elseBB);

        builder->SetInsertPoint(thenBB);
        generateBlock(dynamic_cast<BlockStmt *>(node->thenBlock.get()));
        if (!builder->GetInsertBlock()->getTerminator())
        {
            builder->CreateBr(mergeBB);
        }

        func->insert(func->end(), elseBB);
        builder->SetInsertPoint(elseBB);
        if (node->elseBlock)
        {
            generateBlock(dynamic_cast<BlockStmt *>(node->elseBlock.get()));
        }
        if (!builder->GetInsertBlock()->getTerminator())
        {
            builder->CreateBr(mergeBB);
        }

        func->insert(func->end(), mergeBB);
        builder->SetInsertPoint(mergeBB);

        return nullptr;
    }

    llvm::Value *CodeGenerator::generateWhile(WhileStmt *node)
    {
        llvm::Function *func = builder->GetInsertBlock()->getParent();
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(*context, "whilecond", func);
        llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(*context, "whilebody");
        llvm::BasicBlock *endBB = llvm::BasicBlock::Create(*context, "whileend");

        builder->CreateBr(condBB);

        builder->SetInsertPoint(condBB);
        llvm::Value *cond = generateExpr(node->condition.get());
        builder->CreateCondBr(cond, bodyBB, endBB);

        func->insert(func->end(), bodyBB);
        builder->SetInsertPoint(bodyBB);
        generateBlock(dynamic_cast<BlockStmt *>(node->body.get()));
        if (!builder->GetInsertBlock()->getTerminator())
        {
            builder->CreateBr(condBB);
        }

        func->insert(func->end(), endBB);
        builder->SetInsertPoint(endBB);

        return nullptr;
    }

    llvm::Value *CodeGenerator::generateGoto(GotoStmt *node)
    {
        llvm::Function *func = builder->GetInsertBlock()->getParent();

        std::string labelName = "label_" + node->label;
        llvm::BasicBlock *labelBB = nullptr;

        for (auto &bb : *func)
        {
            if (bb.getName() == labelName)
            {
                labelBB = &bb;
                break;
            }
        }

        if (!labelBB)
        {
            labelBB = llvm::BasicBlock::Create(*context, labelName, func);
        }

        return builder->CreateBr(labelBB);
    }

    llvm::Value *CodeGenerator::generateLabel(LabelStmt *node)
    {
        llvm::Function *func = builder->GetInsertBlock()->getParent();
        std::string labelName = "label_" + node->name;

        llvm::BasicBlock *labelBB = nullptr;
        for (auto &bb : *func)
        {
            if (bb.getName() == labelName)
            {
                labelBB = &bb;
                break;
            }
        }

        if (!labelBB)
        {
            labelBB = llvm::BasicBlock::Create(*context, labelName, func);
        }

        if (!builder->GetInsertBlock()->getTerminator())
        {
            builder->CreateBr(labelBB);
        }
        builder->SetInsertPoint(labelBB);

        if (node->stmt)
        {
            generateStmt(node->stmt.get());
        }

        return nullptr;
    }

    llvm::Function *CodeGenerator::generateFunction(FunctionDef *node)
    {
        std::vector<llvm::Type *> paramTypes;
        for (const auto &t : node->paramTypes)
        {
            paramTypes.push_back(getLLVMType(t));
        }

        llvm::Type *returnType = getLLVMType(node->returnType);
        llvm::FunctionType *funcType = llvm::FunctionType::get(returnType, paramTypes, false);

        llvm::Function *func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            node->name,
            module.get());

        unsigned idx = 0;
        for (auto &arg : func->args())
        {
            if (idx < node->params.size())
            {
                arg.setName(node->params[idx]);
            }
            idx++;
        }

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(*context, "entry", func);
        builder->SetInsertPoint(entry);

        namedAllocas.clear();
        idx = 0;
        for (auto &arg : func->args())
        {
            std::string argName = arg.getName().str();
            llvm::AllocaInst *alloca = createEntryBlockAlloca(func, argName, arg.getType());
            builder->CreateStore(&arg, alloca);
            namedAllocas[argName] = alloca;
            idx++;
        }

        generateBlock(dynamic_cast<BlockStmt *>(node->body.get()));

        if (!builder->GetInsertBlock()->getTerminator())
        {
            if (returnType->isVoidTy())
            {
                builder->CreateRetVoid();
            }
            else
            {
                builder->CreateRet(llvm::ConstantInt::get(returnType, 0));
            }
        }

        llvm::verifyFunction(*func, &llvm::errs());

        return func;
    }

    void CodeGenerator::generate(const Program &program)
    {
        std::vector<llvm::Type *> writeArgs = {
            builder->getInt32Ty(),
            llvm::PointerType::get(builder->getInt8Ty(), 0),
            builder->getInt32Ty()};
        llvm::FunctionType *writeType = llvm::FunctionType::get(
            builder->getInt32Ty(), writeArgs, false);
        llvm::Function::Create(writeType, llvm::Function::ExternalLinkage,
                               "write", module.get());

        for (auto &func : program.functions)
        {
            generateFunction(func.get());
        }

        if (llvm::verifyModule(*module, &llvm::errs()))
        {
            throw std::runtime_error("Module verification failed");
        }
    }

    std::string CodeGenerator::getIR() const
    {
        std::string ir;
        llvm::raw_string_ostream os(ir);
        module->print(os, nullptr);
        return os.str();
    }

} // namespace vexlang