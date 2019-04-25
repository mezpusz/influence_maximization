#pragma once

#include <string>
#include <vector>

class OptionParser
{
    public:
        OptionParser(int argc, char* argv[]);
        bool GetOption(const std::string& option, std::string& value) const;
        bool GetBoolOption(const std::string& option) const;

    private:
        std::vector<std::string> tokens_;
};
