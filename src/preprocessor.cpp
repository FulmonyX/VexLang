#include "../inc/preprocessor.h"
#include "../inc/utils.h"
#include <iostream>
#include <regex>
#include <stack>

Preprocessor::Preprocessor() : lineNumber(0)
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
    output.clear();
    lineNumber = 0;
}

std::string Preprocessor::preprocess(const std::string &mainFile)
{
    clear();
    currentFile = mainFile;
    std::string sourceDir = Utils::getDir(mainFile);
    preprocessFile(mainFile, sourceDir);
    return output;
}

void Preprocessor::preprocessFile(const std::string &path, const std::string &sourceDir)
{
    if (includedFiles.find(path) != includedFiles.end())
    {
        return;
    }
    includedFiles.insert(path);

    std::string content = Utils::readFile(path);
    if (content.empty())
    {
        std::cerr << "Warning: Cannot read file: " << path << std::endl;
        return;
    }

    auto lines = Utils::splitLines(content);
    lineNumber = 0;

    for (const auto &line : lines)
    {
        lineNumber++;
        currentFile = path;
        std::string trimmed = Utils::trim(line);

        if (trimmed.empty())
        {
            output += line + "\n";
            continue;
        }

        if (trimmed[0] == '#')
        {
            if (trimmed.substr(0, 4) == "#inc")
            {
                handleInclude(line, sourceDir);
                continue;
            }
            else if (trimmed.substr(0, 6) == "#macro")
            {
                handleMacro(line);
                continue;
            }
            else if (trimmed.substr(0, 6) == "#undef")
            {
                handleUndef(line);
                continue;
            }
            else if (trimmed.substr(0, 6) == "#ifdef")
            {
                handleIfdef(line);
                continue;
            }
            else if (trimmed.substr(0, 7) == "#ifndef")
            {
                handleIfndef(line);
                continue;
            }
            else if (trimmed.substr(0, 5) == "#else")
            {
                handleElse();
                continue;
            }
            else if (trimmed.substr(0, 5) == "#elif")
            {
                continue;
            }
            else if (trimmed.substr(0, 5) == "#endif")
            {
                handleEndif();
                continue;
            }
            else
            {
                output += line + "\n";
                continue;
            }
        }

        std::string expanded = expandMacros(line);
        output += expanded + "\n";
    }
}

bool Preprocessor::handleInclude(const std::string &line, const std::string &sourceDir)
{
    std::string trimmed = Utils::trim(line);
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
        std::cerr << "Error: Invalid #inc syntax: " << line << std::endl;
        return false;
    }

    std::string fullPath;
    if (isSystem)
    {
        for (const auto &path : systemPaths)
        {
            std::string testPath = Utils::joinPath(path, filename);
            if (Utils::fileExists(testPath))
            {
                fullPath = testPath;
                break;
            }
        }
    }
    else
    {
        fullPath = Utils::joinPath(sourceDir, filename);
        if (!Utils::fileExists(fullPath) && Utils::fileExists(filename))
        {
            fullPath = filename;
        }
    }

    if (fullPath.empty())
    {
        std::cerr << "Error: Cannot find include file: " << filename << std::endl;
        return false;
    }

    std::string includeDir = Utils::getDir(fullPath);
    preprocessFile(fullPath, includeDir);
    return true;
}

void Preprocessor::handleMacro(const std::string &line)
{
    std::string trimmed = Utils::trim(line);
    std::string rest = Utils::trim(trimmed.substr(6));

    if (rest.empty())
    {
        std::cerr << "Error: Invalid #macro syntax: " << line << std::endl;
        return;
    }

    size_t spacePos = rest.find_first_of(" \t");
    if (spacePos == std::string::npos)
    {
        macros[rest] = "";
        return;
    }

    std::string macroName = rest.substr(0, spacePos);
    std::string macroValue = Utils::trim(rest.substr(spacePos));
    macros[macroName] = macroValue;
}

void Preprocessor::handleUndef(const std::string &line)
{
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(6));
    auto it = macros.find(name);
    if (it != macros.end())
    {
        macros.erase(it);
    }
}

void Preprocessor::handleIfdef(const std::string &line)
{
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(6));
    bool defined = macros.find(name) != macros.end();
    if (!defined)
    {
        std::cerr << "Warning: #ifdef " << name << " is false" << std::endl;
    }
}

void Preprocessor::handleIfndef(const std::string &line)
{
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(7));
    bool defined = macros.find(name) != macros.end();
    if (defined)
    {
        std::cerr << "Warning: #ifndef " << name << " is false" << std::endl;
    }
}

void Preprocessor::handleElse()
{
}

void Preprocessor::handleEndif()
{
}

std::string Preprocessor::expandMacros(const std::string &line)
{
    std::string result = line;
    for (const auto &macro : macros)
    {
        const std::string &name = macro.first;
        const std::string &value = macro.second;

        size_t pos = 0;
        while ((pos = result.find(name, pos)) != std::string::npos)
        {
            bool before = (pos > 0 && std::isalnum(result[pos - 1]));
            bool after = (pos + name.length() < result.length() &&
                          std::isalnum(result[pos + name.length()]));

            if (!before && !after)
            {
                result.replace(pos, name.length(), value);
                pos += value.length();
            }
            else
            {
                pos += name.length();
            }
        }
    }
    return result;
}

std::string Preprocessor::findIncludeFile(const std::string &filename, const std::string &sourceDir)
{
    std::string path = Utils::joinPath(sourceDir, filename);
    if (Utils::fileExists(path))
        return path;

    for (const auto &sysPath : systemPaths)
    {
        path = Utils::joinPath(sysPath, filename);
        if (Utils::fileExists(path))
            return path;
    }
    return "";
}