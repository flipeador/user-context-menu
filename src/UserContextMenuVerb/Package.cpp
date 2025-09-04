#include "framework.hpp"

Json LoadPackageCommands()
{
    auto path = L"%:LOCAL%/commands.json";
    std::ifstream file(ExpandEnvironmentStrings(path));
    return file.is_open() ? Json::parse(file) : Json::array();
}
