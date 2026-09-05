#include "../inc/preprocessor.h"
#include "../inc/lexer.h"
#include "../inc/utils.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <source.vl>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];

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

        std::cout << "=== Tokens ===" << std::endl;
        for (const auto &token : tokens)
        {
            std::cout << token.toString() << std::endl;
        }
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