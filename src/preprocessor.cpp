#include "../inc/preprocessor.h"
#include "../inc/utils.h"
#include <iostream>
#include <regex>
#include <algorithm>

Preprocessor::Preprocessor() : lineNumber(0), globalLine(0) {
    systemPaths.push_back("./lib");
}

void Preprocessor::addSystemPath(const std::string& path) {
    systemPaths.push_back(path);
}

void Preprocessor::clear() {
    macros.clear();
    includedFiles.clear();
    while (!condStack.empty()) condStack.pop();
    output.clear();
    lineNumber = 0;
    globalLine = 0;
}

std::string Preprocessor::preprocess(const std::string& mainFile) {
    clear();
    currentFile = mainFile;
    std::string sourceDir = Utils::getDir(mainFile);
    preprocessFile(mainFile, sourceDir);
    return output;
}

void Preprocessor::preprocessFile(const std::string& path, const std::string& sourceDir) {
    std::string normPath = normalizePath(path);
    if (includedFiles.find(normPath) != includedFiles.end()) {
        return;
    }
    includedFiles.insert(normPath);
    
    std::string content = Utils::readFile(path);
    if (content.empty()) {
        std::cerr << "Error: Cannot read file: " << path << std::endl;
        return;
    }
    
    auto lines = Utils::splitLines(content);
    lineNumber = 0;
    
    for (const auto& line : lines) {
        lineNumber++;
        globalLine++;
        currentFile = path;
        processLine(line, sourceDir);
    }
}

void Preprocessor::processLine(const std::string& line, const std::string& sourceDir) {
    std::string trimmed = Utils::trim(line);
    bool inSkipped = isInSkippedBlock();
    
    if (trimmed.empty()) {
        output += line + "\n";
        return;
    }
    
    if (trimmed[0] == '#') {
        if (trimmed.substr(0, 4) == "#inc") {
            if (!inSkipped) {
                handleInclude(line, sourceDir);
            }
            return;
        } else if (trimmed.substr(0, 6) == "#macro") {
            if (!inSkipped) {
                handleMacro(line);
            }
            return;
        } else if (trimmed.substr(0, 6) == "#undef") {
            if (!inSkipped) {
                handleUndef(line);
            }
            return;
        } else if (trimmed.substr(0, 6) == "#ifdef") {
            handleIfdef(line);
            return;
        } else if (trimmed.substr(0, 7) == "#ifndef") {
            handleIfndef(line);
            return;
        } else if (trimmed.substr(0, 5) == "#else") {
            handleElse();
            return;
        } else if (trimmed.substr(0, 5) == "#elif") {
            handleElif(line);
            return;
        } else if (trimmed.substr(0, 5) == "#endif") {
            handleEndif();
            return;
        } else {
            if (!inSkipped) {
                output += line + "\n";
            }
            return;
        }
    }
    
    if (!inSkipped) {
        std::string expanded = expandMacros(line);
        output += expanded + "\n";
    }
}

bool Preprocessor::isInSkippedBlock() {
    if (condStack.empty()) return false;
    return condStack.top().skipped;
}

bool Preprocessor::evaluateIf(const std::string& name) {
    return macros.find(name) != macros.end();
}

void Preprocessor::handleIfdef(const std::string& line) {
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(6));
    bool defined = evaluateIf(name);
    
    bool parentSkipped = isInSkippedBlock();
    bool active = !parentSkipped;
    bool skipped = !defined || parentSkipped;
    
    condStack.push(CondStack(active, skipped, false));
}

void Preprocessor::handleIfndef(const std::string& line) {
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(7));
    bool defined = evaluateIf(name);
    
    bool parentSkipped = isInSkippedBlock();
    bool active = !parentSkipped;
    bool skipped = defined || parentSkipped;
    
    condStack.push(CondStack(active, skipped, false));
}

void Preprocessor::handleElse() {
    if (condStack.empty()) {
        std::cerr << "Error: #else without #if" << std::endl;
        return;
    }
    if (condStack.top().hasElse) {
        std::cerr << "Error: #else after #else" << std::endl;
        return;
    }
    condStack.top().hasElse = true;
    bool parentSkipped = false;
    if (condStack.size() > 1) {
        auto temp = condStack;
        temp.pop();
        parentSkipped = temp.top().skipped;
    }
    bool currentActive = condStack.top().active;
    condStack.top().skipped = parentSkipped || !currentActive;
}

void Preprocessor::handleElif(const std::string& line) {
    if (condStack.empty()) {
        std::cerr << "Error: #elif without #if" << std::endl;
        return;
    }
    if (condStack.top().hasElse) {
        std::cerr << "Error: #elif after #else" << std::endl;
        return;
    }
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(5));
    bool defined = evaluateIf(name);
    
    bool parentSkipped = false;
    if (condStack.size() > 1) {
        auto temp = condStack;
        temp.pop();
        parentSkipped = temp.top().skipped;
    }
    bool currentActive = condStack.top().active;
    if (currentActive && !parentSkipped) {
        condStack.top().skipped = !defined;
    } else {
        condStack.top().skipped = true;
    }
    condStack.top().active = true;
}

void Preprocessor::handleEndif() {
    if (condStack.empty()) {
        std::cerr << "Error: #endif without #if" << std::endl;
        return;
    }
    condStack.pop();
}

bool Preprocessor::handleInclude(const std::string& line, const std::string& sourceDir) {
    std::string trimmed = Utils::trim(line);
    std::string filename;
    bool isSystem = false;
    
    size_t startQuote = trimmed.find('"');
    size_t startAngle = trimmed.find('<');
    
    if (startQuote != std::string::npos) {
        size_t endQuote = trimmed.find('"', startQuote + 1);
        if (endQuote != std::string::npos) {
            filename = trimmed.substr(startQuote + 1, endQuote - startQuote - 1);
            isSystem = false;
        }
    } else if (startAngle != std::string::npos) {
        size_t endAngle = trimmed.find('>', startAngle + 1);
        if (endAngle != std::string::npos) {
            filename = trimmed.substr(startAngle + 1, endAngle - startAngle - 1);
            isSystem = true;
        }
    }
    
    if (filename.empty()) {
        std::cerr << "Error: Invalid #inc syntax: " << line << std::endl;
        return false;
    }
    
    filename = expandMacros(filename);
    
    std::string fullPath;
    if (isSystem) {
        for (const auto& path : systemPaths) {
            std::string testPath = Utils::joinPath(path, filename);
            if (Utils::fileExists(testPath)) {
                fullPath = testPath;
                break;
            }
        }
    } else {
        fullPath = Utils::joinPath(sourceDir, filename);
        if (!Utils::fileExists(fullPath) && Utils::fileExists(filename)) {
            fullPath = filename;
        }
    }
    
    if (fullPath.empty()) {
        std::cerr << "Fatal Error: Cannot find include file: " << filename << std::endl;
        exit(1);
    }
    
    std::string includeDir = Utils::getDir(fullPath);
    preprocessFile(fullPath, includeDir);
    return true;
}

void Preprocessor::handleMacro(const std::string& line) {
    std::string trimmed = Utils::trim(line);
    std::string rest = Utils::trim(trimmed.substr(6));
    
    if (rest.empty()) {
        std::cerr << "Error: Invalid #macro syntax: " << line << std::endl;
        return;
    }
    
    size_t spacePos = rest.find_first_of(" \t");
    if (spacePos == std::string::npos) {
        macros[rest] = "";
        return;
    }
    
    std::string macroName = rest.substr(0, spacePos);
    std::string macroValue = Utils::trim(rest.substr(spacePos));
    macros[macroName] = macroValue;
}

void Preprocessor::handleUndef(const std::string& line) {
    std::string trimmed = Utils::trim(line);
    std::string name = Utils::trim(trimmed.substr(6));
    auto it = macros.find(name);
    if (it != macros.end()) {
        macros.erase(it);
    }
}

std::string Preprocessor::expandMacros(const std::string& line) {
    std::string result = line;
    bool changed = true;
    int maxIterations = 100;
    
    while (changed && maxIterations-- > 0) {
        changed = false;
        for (const auto& macro : macros) {
            const std::string& name = macro.first;
            const std::string& value = macro.second;
            
            size_t pos = 0;
            while ((pos = result.find(name, pos)) != std::string::npos) {
                bool inString = false;
                bool inComment = false;
                for (size_t i = 0; i < pos; i++) {
                    if (result[i] == '"' && (i == 0 || result[i-1] != '\\')) {
                        inString = !inString;
                    }
                    if (i > 0 && result[i-1] == '/' && result[i] == '/') {
                        inComment = true;
                    }
                }
                
                bool before = (pos > 0 && std::isalnum(result[pos - 1]));
                bool after = (pos + name.length() < result.length() && 
                             std::isalnum(result[pos + name.length()]));
                
                if (!inString && !inComment && !before && !after) {
                    std::string expanded = value;
                    size_t start = pos;
                    size_t end = pos + name.length();
                    result.replace(start, end - start, expanded);
                    pos += expanded.length();
                    changed = true;
                } else {
                    pos += name.length();
                }
            }
        }
    }
    
    return result;
}

std::string Preprocessor::findIncludeFile(const std::string& filename, const std::string& sourceDir) {
    std::string path = Utils::joinPath(sourceDir, filename);
    if (Utils::fileExists(path)) return path;
    
    for (const auto& sysPath : systemPaths) {
        path = Utils::joinPath(sysPath, filename);
        if (Utils::fileExists(path)) return path;
    }
    return "";
}

std::string Preprocessor::normalizePath(const std::string& path) {
    std::string result = path;
    size_t pos = 0;
    while ((pos = result.find("./", pos)) != std::string::npos) {
        if (pos == 0 || result[pos-1] == '/') {
            result.erase(pos, 2);
        } else {
            pos++;
        }
    }
    pos = 0;
    while ((pos = result.find("/./", pos)) != std::string::npos) {
        result.erase(pos, 2);
    }
    return result;
}