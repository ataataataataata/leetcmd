#include "include/html_parser.h"
#include <map>

std::string stripHtml(std::string rawHtml)
{
    std::map<std::string, std::string> htmlEntities = {
        {"p", "\n"},
        {"br", "\n"},
        {"li", "\n- "},
        {"pre", "\n"}};

    std::string result;
    std::string currentTag;
    for (int i = 0; i < rawHtml.size(); i++)
    {
        if (rawHtml[i] == '<')
        {
            i++;
            currentTag.clear();
            while (i < rawHtml.size() && rawHtml[i] != '>' && rawHtml[i] != ' ')
            {
                currentTag += rawHtml[i];
                i++;
            }

            if (i < rawHtml.size() && rawHtml[i] == ' ')
            {

                while (i < rawHtml.size() && rawHtml[i] != '>')
                {
                    i++;
                }
            }

            auto htmlEntityMatch = htmlEntities.find(currentTag);

            if (htmlEntityMatch != htmlEntities.end())
            {
                result += htmlEntityMatch->second;
            }
        }
        else
        {
            result += rawHtml[i];
        }
    }

    std::map<std::string, std::string> htmlSpecialChars = {
        {"&quot;", "\""},
        {"&nbsp;", " "},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&amp;", "&"},
        {"&#39;", "'"}};

    for (auto &entity : htmlSpecialChars)
    {
        size_t pos = 0;
        while ((pos = result.find(entity.first, pos)) != std::string::npos)
        {
            result.replace(pos, entity.first.length(), entity.second);
            pos += entity.second.length();
        }
    }

    return result;
}