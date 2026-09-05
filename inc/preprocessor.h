#ifndef VEXLANG_INC_PREPROCESSOR_H
#define VEXLANG_INC_PREPROCESSOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <stdexcept>

namespace vexlang
{

    class PreprocessorError : public std::runtime_error
    {
    public:
        explicit PreprocessorError(const std::string &msg);
    };

    struct CondState
    {
        bool inActiveBlock;
        bool skipToEndif;
        bool hasElse;
        CondState(bool active = true, bool skip = false, bool else_ = false);
    };

    class Preprocessor
    {
    public:
        Preprocessor();

        void addSystemPath(const std::string &path);
        std::string preprocess(const std::string &mainFile);
        const std::string &getOutput() const;
        void clear();

    private:
        std::unordered_map<std::string, std::string> macros;
        std::unordered_set<std::string> includedFiles;
        std::vector<std::string> systemPaths;
        std::stack<CondState> condStack;
        std::string output;
        std::string currentFile;
        int lineNumber;
        int globalLine;

        void processLine(const std::string &line, const std::string &sourceDir);
        bool handleInclude(const std::string &line, const std::string &sourceDir);
        void handleMacro(const std::string &line);
        void handleUndef(const std::string &line);
        void handleIfdef(const std::string &line);
        void handleIfndef(const std::string &line);
        void handleElse();
        void handleElif(const std::string &line);
        void handleEndif();
        std::string expandMacros(const std::string &line);
        std::string expandMacrosRecursive(const std::string &line, std::unordered_set<std::string> &expanding);
        std::string findIncludeFile(const std::string &filename, const std::string &sourceDir);
        void preprocessFile(const std::string &path, const std::string &sourceDir);
        std::string normalizePath(const std::string &path);
        bool isInSkippedBlock() const;
        bool evaluateIf(const std::string &name) const;
        bool isWholeWord(const std::string &str, size_t pos, const std::string &word) const;
        bool isInStringOrComment(const std::string &str, size_t pos) const;
    };

} // namespace vexlang

#endif