#ifndef MULTIPARTPARSER_HPP
#define MULTIPARTPARSER_HPP

#include <string>
#include <vector>

struct Part
{
    std::string filename;
    std::vector<char> data;
};

class MultipartParser
{
    private:
        std::string boundary;
        std::vector<char> body;
        size_t pos;
        std::string readLine();
        bool matchAt(size_t position, const std::string& s) const;
    public:
        MultipartParser(const std::string& b, const std::vector<char>& bodyContent);
        bool hasNextPart();
        Part nextPart();
};

#endif
