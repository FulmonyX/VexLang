#include "../inc/utils.h"
#include <fstream>
#include <sstream>
#include <cctype>

namespace vexlang
{
    namespace utils
    {

        std::string readFile(const std::string &path)
        {
            std::ifstream file(path);
            if (!file.is_open())
            {
                return "";
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        bool fileExists(const std::string &path)
        {
            std::ifstream file(path);
            return file.good();
        }

        std::string trim(const std::string &str)
        {
            size_t start = str.find_first_not_of(" \t\n\r");
            if (start == std::string::npos)
                return "";
            size_t end = str.find_last_not_of(" \t\n\r");
            return str.substr(start, end - start + 1);
        }

        std::string getDir(const std::string &path)
        {
            size_t pos = path.find_last_of("/\\");
            if (pos == std::string::npos)
                return ".";
            return path.substr(0, pos);
        }

        std::string joinPath(const std::string &dir, const std::string &file)
        {
            if (dir == "." || dir.empty())
                return file;
            return dir + "/" + file;
        }

        std::vector<std::string> splitLines(const std::string &content)
        {
            std::vector<std::string> lines;
            std::istringstream stream(content);
            std::string line;
            while (std::getline(stream, line))
            {
                lines.push_back(line);
            }
            return lines;
        }

        std::string normalizePath(const std::string &path)
        {
            std::string result = path;
            size_t pos = 0;
            while ((pos = result.find("./", pos)) != std::string::npos)
            {
                if (pos == 0 || result[pos - 1] == '/')
                {
                    result.erase(pos, 2);
                }
                else
                {
                    pos++;
                }
            }
            pos = 0;
            while ((pos = result.find("/./", pos)) != std::string::npos)
            {
                result.erase(pos, 2);
            }
            return result;
        }

    } // namespace utils
} // namespace vexlang