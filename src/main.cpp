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

    if (!Utils::fileExists(inputFile))
    {
        std::cerr << "Error: File not found: " << inputFile << std::endl;
        return 1;
    }

    std::string exeDir = Utils::getDir(argv[0]);

    Preprocessor preprocessor;
    preprocessor.addSystemPath(Utils::joinPath(exeDir, "lib"));
    std::string code = preprocessor.preprocess(inputFile);

    Lexer lexer(code);
    std::vector<Token> tokens = lexer.tokenizeAll();

    std::cout << "=== Tokens ===" << std::endl;
    for (const auto &token : tokens)
    {
        std::cout << token.toString() << std::endl;
    }

    return 0;
}