#include "optionparser.h"

OptionParser::OptionParser(int argc, char* argv[])
    : tokens_()
{
    for (int i = 1; i < argc; ++i)
    {
        tokens_.push_back(std::string(argv[i]));
    }
}

bool OptionParser::GetBoolOption(const std::string& option) const
{
    return std::find(tokens_.begin(), tokens_.end(), option)
        != tokens_.end();
}
