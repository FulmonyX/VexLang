#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>

struct CondStack {
    bool active;
    bool skipped;
    bool hasElse;
    CondStack(bool a = true, bool s = false, bool e = false)
        : active(a), skipped(s), hasElse(e) {}
};

class Preprocessor {
private:
    std::unordered_map<std::string, std::string> macros;
    std::unordered_set<std::string> includedFiles;
    std::vector<std::string> systemPaths;
    std::stack<CondStack> condStack;
    std::string output;
    std::string currentFile;
    int lineNumber;
    int globalLine;
    
    void processLine(const std::string& line, const std::string& sourceDir);
    bool handleInclude(const std::string& line, const std::string& sourceDir);
    void handleMacro(const std::string& line);
    void handleUndef(const std::string& line);
    void handleIfdef(const std::string& line);
    void handleIfndef(const std::string& line);
    void handleElse();
    void handleElif(const std::string& line);
    void handleEndif();
    std::string expandMacros(const std::string& line);
    std::string findIncludeFile(const std::string& filename, const std::string& sourceDir);
    void preprocessFile(const std::string& path, const std::string& sourceDir);
    std::string normalizePath(const std::string& path);
    bool isInSkippedBlock();
    bool evaluateIf(const std::string& name);
    
public:
    Preprocessor();
    void addSystemPath(const std::string& path);
    std::string preprocess(const std::string& mainFile);
    const std::string& getOutput() const { return output; }
    void clear();
};

#endif