#include "../include/preprocessor.h"
#include "../include/utils.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source.vl>" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    
    if (!Utils::fileExists(inputFile)) {
        std::cerr << "Error: File not found: " << inputFile << std::endl;
        return 1;
    }
    
    std::string exeDir = Utils::getDir(argv[0]);
    
    Preprocessor preprocessor;
    preprocessor.addSystemPath(Utils::joinPath(exeDir, "lib"));
    
    std::cout << preprocessor.preprocess(inputFile);
    
    return 0;
}