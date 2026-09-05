#ifndef VEXLANG_INC_UTILS_H
#define VEXLANG_INC_UTILS_H

#include <string>
#include <vector>

namespace vexlang
{
    namespace utils
    {

        std::string readFile(const std::string &path);
        bool fileExists(const std::string &path);
        std::string trim(const std::string &str);
        std::string getDir(const std::string &path);
        std::string joinPath(const std::string &dir, const std::string &file);
        std::vector<std::string> splitLines(const std::string &content);
        std::string normalizePath(const std::string &path);

    } // namespace utils
} // namespace vexlang

#endif