#include "../inc/preprocessor.h"
#include "../inc/lexer.h"
#include "../inc/parser.h"
#include "../inc/codegen.h"
#include "../inc/utils.h"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source.vl> [-o output]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = "output.ll";

    for (int i = 2; i < argc; i++)
    {
        if (std::string(argv[i]) == "-o" && i + 1 < argc)
        {
            outputFile = argv[i + 1];
        }
    }

    if (!vexlang::utils::fileExists(inputFile))
    {
        std::cerr << "Error: File not found: " << inputFile << std::endl;
        return 1;
    }

    std::string exeDir = vexlang::utils::getDir(argv[0]);

    try
    {
        vexlang::Preprocessor preprocessor;
        preprocessor.addSystemPath(vexlang::utils::joinPath(exeDir, "lib"));
        std::string code = preprocessor.preprocess(inputFile);

        vexlang::Lexer lexer(code);
        std::vector<vexlang::Token> tokens = lexer.tokenizeAll();

        vexlang::Parser parser(tokens);
        std::unique_ptr<vexlang::Program> program = parser.parse();
        std::cout << " Parsing successful! Found "
                  << program->functions.size() << " function(s)" << std::endl;

        vexlang::CodeGenerator codegen;
        codegen.generate(*program);
        std::string ir = codegen.getIR();

        std::ofstream outFile(outputFile);
        outFile << ir;
        outFile.close();

        std::cout << " IR generated: " << outputFile << std::endl;
        std::cout << "\n=== LLVM IR ===" << std::endl;
        std::cout << ir << std::endl;
    }
    catch (const vexlang::PreprocessorError &e)
    {
        std::cerr << "Preprocessor error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}