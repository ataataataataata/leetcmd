#pragma once

#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string>

namespace fs = std::filesystem;

inline fs::path getConfigPath()
{
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"))
        return fs::path(appdata) / "leetcmd" / "config.txt";

    return "leetcmd_config.txt";
#else
    if (const char* home = std::getenv("HOME"))
        return fs::path(home) / ".leetcmd_config";

    return ".leetcmd_config";
#endif
}

inline const fs::path configPath = getConfigPath();

bool checkConfig();
std::vector<std::string> readConfig();
void createConfig(std::string leetcode_session, std::string csrftoken);