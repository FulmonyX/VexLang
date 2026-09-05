#include "../inc/preprocessor.h"
#include "../inc/utils.h"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace vexlang
{

    PreprocessorError::PreprocessorError(const std::string &msg)
        : std::runtime_error(msg) {}

    CondState::CondState(bool active, bool skip, bool else_)
        : inActiveBlock(active), skipToEndif(skip), hasElse(else_) {}

    Preprocessor::Preprocessor() : lineNumber(0), globalLine(0)
    {
        systemPaths.push_back("./lib");
    }

    void Preprocessor::addSystemPath(const std::string &path)
    {
        systemPaths.push_back(path);
    }

    void Preprocessor::clear()
    {
        macros.clear();
        includedFiles.clear();
        while (!condStack.empty())
            condStack.pop();
        output.clear();
        lineNumber = 0;
        globalLine = 0;
    }

    const std::string &Preprocessor::getOutput() const
    {
        return output;
    }

    std::string Preprocessor::preprocess(const std::string &mainFile)
    {
        clear();
        currentFile = mainFile;
        std::string sourceDir = utils::getDir(mainFile);
        preprocessFile(mainFile, sourceDir);
        return output;
    }

    void Preprocessor::preprocessFile(const std::string &path, const std::string &sourceDir)
    {
        std::string normPath = utils::normalizePath(path);
        if (includedFiles.find(normPath) != includedFiles.end())
        {
            return;
        }
        includedFiles.insert(normPath);

        std::string content = utils::readFile(path);
        if (content.empty())
        {
            throw PreprocessorError("Cannot read file: " + path);
        }

        auto lines = utils::splitLines(content);
        lineNumber = 0;

        for (const auto &line : lines)
        {
            lineNumber++;
            globalLine++;
            currentFile = path;
            processLine(line, sourceDir);
        }
    }

    void Preprocessor::processLine(const std::string &line, const std::string &sourceDir)
    {
        std::string trimmed = utils::trim(line);
        bool inSkipped = isInSkippedBlock();

        if (trimmed.empty())
        {
            output += line + "\n";
            return;
        }

        if (trimmed[0] == '#')
        {
            if (trimmed.substr(0, 4) == "#inc")
            {
                if (!inSkipped)
                {
                    handleInclude(line, sourceDir);
                }
                return;
            }
            else if (trimmed.substr(0, 6) == "#macro")
            {
                if (!inSkipped)
                {
                    handleMacro(line);
                }
                return;
            }
            else if (trimmed.substr(0, 6) == "#undef")
            {
                if (!inSkipped)
                {
                    handleUndef(line);
                }
                return;
            }
            else if (trimmed.substr(0, 6) == "#ifdef")
            {
                handleIfdef(line);
                return;
            }
            else if (trimmed.substr(0, 7) == "#ifndef")
            {
                handleIfndef(line);
                return;
            }
            else if (trimmed.substr(0, 5) == "#else")
            {
                handleElse();
                return;
            }
            else if (trimmed.substr(0, 5) == "#elif")
            {
                handleElif(line);
                return;
            }
            else if (trimmed.substr(0, 5) == "#endif")
            {
                handleEndif();
                return;
            }
            else
            {
                if (!inSkipped)
                {
                    output += line + "\n";
                }
                return;
            }
        }

        if (!inSkipped)
        {
            std::unordered_set<std::string> expanding;
            std::string expanded = expandMacrosRecursive(line, expanding);
            output += expanded + "\n";
        }
    }

    bool Preprocessor::isInSkippedBlock() const
    {
        if (condStack.empty())
            return false;
        return condStack.top().skipToEndif;
    }

    bool Preprocessor::evaluateIf(const std::string &name) const
    {
        return macros.find(name) != macros.end();
    }

    void Preprocessor::handleIfdef(const std::string &line)
    {
        std::string trimmed = utils::trim(line);
        std::string name = utils::trim(trimmed.substr(6));
        bool defined = evaluateIf(name);

        bool parentSkip = condStack.empty() ? false : condStack.top().skipToEndif;
        CondState state;
        state.inActiveBlock = !parentSkip && defined;
        state.skipToEndif = parentSkip || !defined;
        state.hasElse = false;
        condStack.push(state);
    }

    void Preprocessor::handleIfndef(const std::string &line)
    {
        std::string trimmed = utils::trim(line);
        std::string name = utils::trim(trimmed.substr(7));
        bool defined = evaluateIf(name);

        bool parentSkip = condStack.empty() ? false : condStack.top().skipToEndif;
        CondState state;
        state.inActiveBlock = !parentSkip && !defined;
        state.skipToEndif = parentSkip || defined;
        state.hasElse = false;
        condStack.push(state);
    }

    void Preprocessor::handleElse()
    {
        if (condStack.empty())
        {
            throw PreprocessorError("#else without #if");
        }
        if (condStack.top().hasElse)
        {
            throw PreprocessorError("#else after #else");
        }
        condStack.top().hasElse = true;
        bool parentSkip = condStack.size() > 1 ? condStack.top().skipToEndif : false;
        bool currentActive = condStack.top().inActiveBlock;
        condStack.top().skipToEndif = parentSkip || currentActive;
        condStack.top().inActiveBlock = !parentSkip && !currentActive;
    }

    void Preprocessor::handleElif(const std::string &line)
    {
        if (condStack.empty())
        {
            throw PreprocessorError("#elif without #if");
        }
        if (condStack.top().hasElse)
        {
            throw PreprocessorError("#elif after #else");
        }
        std::string trimmed = utils::trim(line);
        std::string name = utils::trim(trimmed.substr(5));
        bool defined = evaluateIf(name);

        bool parentSkip = condStack.size() > 1 ? condStack.top().skipToEndif : false;
        bool currentActive = condStack.top().inActiveBlock;
        if (currentActive && !parentSkip)
        {
            condStack.top().skipToEndif = !defined;
        }
        else
        {
            condStack.top().skipToEndif = true;
        }
        condStack.top().inActiveBlock = !parentSkip && defined;
    }

    void Preprocessor::handleEndif()
    {
        if (condStack.empty())
        {
            throw PreprocessorError("#endif without #if");
        }
        condStack.pop();
    }

    bool Preprocessor::handleInclude(const std::string &line, const std::string &sourceDir)
    {
        std::string trimmed = utils::trim(line);
        std::string filename;
        bool isSystem = false;

        size_t startQuote = trimmed.find('"');
        size_t startAngle = trimmed.find('<');

        if (startQuote != std::string::npos)
        {
            size_t endQuote = trimmed.find('"', startQuote + 1);
            if (endQuote != std::string::npos)
            {
                filename = trimmed.substr(startQuote + 1, endQuote - startQuote - 1);
                isSystem = false;
            }
        }
        else if (startAngle != std::string::npos)
        {
            size_t endAngle = trimmed.find('>', startAngle + 1);
            if (endAngle != std::string::npos)
            {
                filename = trimmed.substr(startAngle + 1, endAngle - startAngle - 1);
                isSystem = true;
            }
        }

        if (filename.empty())
        {
            throw PreprocessorError("Invalid #inc syntax: " + line);
        }

        std::unordered_set<std::string> expanding;
        filename = expandMacrosRecursive(filename, expanding);

        std::string fullPath;
        if (isSystem)
        {
            for (const auto &path : systemPaths)
            {
                std::string testPath = utils::joinPath(path, filename);
                if (utils::fileExists(testPath))
                {
                    fullPath = testPath;
                    break;
                }
            }
        }
        else
        {
            fullPath = utils::joinPath(sourceDir, filename);
            if (!utils::fileExists(fullPath) && utils::fileExists(filename))
            {
                fullPath = filename;
            }
        }

        if (fullPath.empty())
        {
            throw PreprocessorError("Cannot find include file: " + filename);
        }

        std::string includeDir = utils::getDir(fullPath);
        preprocessFile(fullPath, includeDir);
        return true;
    }

    void Preprocessor::handleMacro(const std::string &line)
    {
        std::string trimmed = utils::trim(line);
        std::string rest = utils::trim(trimmed.substr(6));

        if (rest.empty())
        {
            throw PreprocessorError("Invalid #macro syntax: " + line);
        }

        size_t spacePos = rest.find_first_of(" \t");
        if (spacePos == std::string::npos)
        {
            macros[rest] = "";
            return;
        }

        std::string macroName = rest.substr(0, spacePos);
        std::string macroValue = utils::trim(rest.substr(spacePos));
        macros[macroName] = macroValue;
    }

    void Preprocessor::handleUndef(const std::string &line)
    {
        std::string trimmed = utils::trim(line);
        std::string name = utils::trim(trimmed.substr(6));
        auto it = macros.find(name);
        if (it != macros.end())
        {
            macros.erase(it);
        }
    }

    bool Preprocessor::isWholeWord(const std::string &str, size_t pos, const std::string &word) const
    {
        bool before = (pos > 0 && std::isalnum(str[pos - 1]));
        bool after = (pos + word.length() < str.length() &&
                      std::isalnum(str[pos + word.length()]));
        return !before && !after;
    }

    bool Preprocessor::isInStringOrComment(const std::string &str, size_t pos) const
    {
        bool inString = false;
        bool inComment = false;
        for (size_t i = 0; i < pos; i++)
        {
            if (str[i] == '"' && (i == 0 || str[i - 1] != '\\'))
            {
                inString = !inString;
            }
            if (i > 0 && str[i - 1] == '/' && str[i] == '/')
            {
                inComment = true;
            }
        }
        return inString || inComment;
    }

    std::string Preprocessor::expandMacrosRecursive(const std::string &line,
                                                    std::unordered_set<std::string> &expanding)
    {
        std::string result = line;
        bool changed = true;
        int maxIterations = 100;

        while (changed && maxIterations-- > 0)
        {
            changed = false;
            for (const auto &macro : macros)
            {
                const std::string &name = macro.first;
                const std::string &value = macro.second;

                if (expanding.find(name) != expanding.end())
                {
                    continue;
                }

                size_t pos = 0;
                while ((pos = result.find(name, pos)) != std::string::npos)
                {
                    if (isInStringOrComment(result, pos) || !isWholeWord(result, pos, name))
                    {
                        pos += name.length();
                        continue;
                    }

                    expanding.insert(name);
                    std::string expanded = expandMacrosRecursive(value, expanding);
                    expanding.erase(name);

                    result.replace(pos, name.length(), expanded);
                    pos += expanded.length();
                    changed = true;
                }
            }
        }
        return result;
    }

    std::string Preprocessor::expandMacros(const std::string &line)
    {
        std::unordered_set<std::string> expanding;
        return expandMacrosRecursive(line, expanding);
    }

    std::string Preprocessor::findIncludeFile(const std::string &filename,
                                              const std::string &sourceDir)
    {
        std::string path = utils::joinPath(sourceDir, filename);
        if (utils::fileExists(path))
            return path;

        for (const auto &sysPath : systemPaths)
        {
            path = utils::joinPath(sysPath, filename);
            if (utils::fileExists(path))
                return path;
        }
        return "";
    }

    std::string Preprocessor::normalizePath(const std::string &path)
    {
        return utils::normalizePath(path);
    }

} // namespace vexlang