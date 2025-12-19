#include "MultipartParser.hpp"
#include <algorithm>
#include <stdexcept>


MultipartParser::MultipartParser(const std::string& b,
                                 const std::vector<char>& bodyContent)
    : boundary(b), body(bodyContent), pos(0)
{
    if (body.size() < boundary.size() || !matchAt(0, boundary))
        throw std::runtime_error("Bad multipart format");
}


std::string MultipartParser::readLine()
{
    if (pos >= body.size())
        return "";
    size_t start = pos;
    while (pos + 1 < body.size())
    {
        if (body[pos] == '\r' && body[pos + 1] == '\n')
        {
            std::string line(body.begin() + start, body.begin() + pos);
            pos += 2;
            return (line);
        }
        pos++;
    }
    std::string line(body.begin() + start, body.end());
    pos = body.size();
    return (line);
}

bool MultipartParser::matchAt(size_t position, const std::string& s) const
{
    if (position + s.size() > body.size())
        return (false);
    for (size_t i = 0; i < s.size(); i++)
    {
        if (body[position + i] != s[i])
            return (false);
    }
    return (true);
}

bool MultipartParser::hasNextPart()
{
    return matchAt(pos, boundary);
}

Part MultipartParser::nextPart()
{
    Part part;
    bool hasDisposition = false;


    size_t startBoundary = std::search(
        body.begin() + pos,
        body.end(),
        boundary.begin(),
        boundary.end()
    ) - body.begin();
    if (startBoundary >= body.size()) return part;

    pos = startBoundary + boundary.size();
    if (pos + 2 < body.size() && body[pos] == '\r' && body[pos+1] == '\n')
        pos += 2;

    std::string line;

    while (!(line = readLine()).empty())
    {
        if (line.find("Content-Disposition:") == 0)
            hasDisposition = true;

        size_t fn = line.find("filename=");
        if (fn != std::string::npos)
        {
            part.filename = line.substr(fn + 9);

            size_t end = part.filename.find(';');
            if (end != std::string::npos)
                part.filename = part.filename.substr(0, end);

            if (!part.filename.empty() && part.filename[0] == '"')
                part.filename.erase(0, 1);
            if (!part.filename.empty() && part.filename[part.filename.size() - 1] == '"')
                part.filename.erase(part.filename.size() - 1, 1);

            part.filename.erase(0, part.filename.find_first_not_of(" \t"));
            part.filename.erase(part.filename.find_last_not_of(" \t") + 1);
        }
    }

    if (!hasDisposition)
    {
        pos = body.size();
        return Part();
    }


    size_t endBoundary = std::search(
        body.begin() + pos,
        body.end(),
        boundary.begin(),
        boundary.end()
    ) - body.begin();
    if (endBoundary + boundary.size() + 2 <= body.size() &&
        matchAt(endBoundary + boundary.size(), "--"))
    {
        pos = body.size();
        return part;
    }


    if (endBoundary > pos)
    {
        size_t dataEnd = endBoundary;
        if (dataEnd >= 2 && body[dataEnd-2] == '\r' && body[dataEnd-1] == '\n')
            dataEnd -= 2;
        part.data.assign(body.begin() + pos, body.begin() + dataEnd);
        pos = endBoundary;
    }

    pos = body.size();
    return part;
}