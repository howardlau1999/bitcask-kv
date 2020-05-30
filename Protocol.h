#pragma once
#include <vector>
#include <string>

class Protocol {
    static std::vector<std::string> splitSpace(std::string const& aString);
public:
    static bool parseRequestLine(std::string const& aRequestLine, std::vector<std::string>& outResult);
};