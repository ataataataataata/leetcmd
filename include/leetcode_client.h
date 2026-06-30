#ifndef LEETCMD_LEETCODE_CLIENT_H
#define LEETCMD_LEETCODE_CLIENT_H

#include <nlohmann/json.hpp>
#include <vector>

struct question {
    int id;
    std::string title;
    std::string titleSlug;
    std::string difficulty;
    std::string status;
    bool paidOnly;
    std::vector<std::string> topicTags;
};



const std::string url ="https://leetcode.com/graphql";

std::vector<question> getAllQuestions();
void printGetAllQuestions();


#endif //LEETCMD_LEETCODE_CLIENT_H
