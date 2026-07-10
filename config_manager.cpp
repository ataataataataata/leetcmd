#include "include/config_manager.h"

bool checkConfig()
{
    auto tokens = readConfig();

    return tokens.size() == 2 &&
           !tokens[0].empty() &&
           !tokens[1].empty();
}

void createConfig(std::string leetcode_session, std::string csrftoken)
{
#ifdef _WIN32
    // Windows'ta AppData/Roaming/leetcmd klasörünü oluştur.
    fs::create_directories(configPath.parent_path());
#endif

    std::ofstream file(configPath);

    if (!file.is_open())
        throw std::runtime_error("Failed to create config file: " + configPath.string());

    file << leetcode_session << '\n';
    file << csrftoken << '\n';
}

std::vector<std::string> readConfig()
{
    std::ifstream file(configPath);

    if (!file.is_open())
        return {};

    std::string session;
    std::string csrf;

    if (!std::getline(file, session))
        return {};

    if (!std::getline(file, csrf))
        return {};
        

    return {session, csrf};
}