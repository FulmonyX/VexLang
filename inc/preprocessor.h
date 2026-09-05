#ifndef VEXLANG_INC_PREPROCESSOR_H
#define VEXLANG_INC_PREPROCESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Preprocessor
{
private:
    std::unordered_map<std::string, std::string> macros;
    std::unordered_set<std::string> includedFiles;
    std::vector<std::string> systemPaths;
    std::string output;
    std::string currentFile;
    int lineNumber;

    void processLine(const std::string &line, const std::string &sourceDir);
    bool handleInclude(const std::string &line, const std::string &sourceDir);
    void handleMacro(const std::string &line);
    void handleUndef(const std::string &line);
    void handleIfdef(const std::string &line);
    void handleIfndef(const std::string &line);
    void handleEndif();
    void handleElse();
    std::string expandMacros(const std::string &line);
    std::string findIncludeFile(const std::string &filename, const std::string &sourceDir);
    void preprocessFile(const std::string &path, const std::string &sourceDir);

public:
    Preprocessor();
    void addSystemPath(const std::string &path);
    std::string preprocess(const std::string &mainFile);
    const std::string &getOutput() const { return output; }
    void clear();
};

#endif