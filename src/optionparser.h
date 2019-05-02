#pragma once

#include <string>
#include <vector>
#include <sstream>

class OptionParser
{
    public:
        OptionParser(int argc, char* argv[]);
        bool GetBoolOption(const std::string& option) const;

        template <typename T>
        bool GetOption(const std::string& option, T& value) const
        {
            auto it = std::find(tokens_.begin(), tokens_.end(), option);
            if (it != tokens_.end() && ++it != tokens_.end())
            {
                std::istringstream is(*it);
                is >> value;
                return true;
            }
            return false;
        }

    private:
        std::vector<std::string> tokens_;
};
