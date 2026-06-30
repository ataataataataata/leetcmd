#include "include/leetcode_client.h"
#include "include/http_client.h"


std::vector<question> getAllQuestions() {
    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=SESSION_TOKEN_REDACTED; csrftoken=CSRF_TOKEN_REDACTED");
    headers.push_back("x-csrftoken:CSRF_TOKEN_REDACTED");
    headers.push_back("Referer: https://leetcode.com/problemset/");
    headers.push_back("Origin: https://leetcode.com");

    std::string bodyText = R"JSON({"operationName":"problemsetQuestionListV2","query":"\n    query problemsetQuestionListV2($filters: QuestionFilterInput, $limit: Int, $searchKeyword: String, $skip: Int, $sortBy: QuestionSortByInput, $categorySlug: String) {\n  problemsetQuestionListV2(\n    filters: $filters\n    limit: $limit\n    searchKeyword: $searchKeyword\n    skip: $skip\n    sortBy: $sortBy\n    categorySlug: $categorySlug\n  ) {\n    questions {\n      id\n      titleSlug\n      title\n      translatedTitle\n      questionFrontendId\n      paidOnly\n      difficulty\n      topicTags {\n        name\n        slug\n        nameTranslated\n      }\n      status\n      isInMyFavorites\n      frequency\n      acRate\n      contestPoint\n    }\n    totalLength\n    finishedLength\n    hasMore\n  }\n}\n    ","variables":{"skip":0,"limit":100,"categorySlug":"all-code-essentials","searchKeyword":"","sortBy":{"sortField":"CUSTOM","sortOrder":"ASCENDING"},"filters":{"filterCombineType":"ALL","statusFilter":{"questionStatuses":[],"operator":"IS"}}}})JSON";

    response = httpPost(url, bodyText, headers);


    nlohmann::json j = nlohmann::json::parse(response);


    std::vector<question> questionList;
    auto questions = j["data"]["problemsetQuestionListV2"]["questions"];
    for (auto& q: questions) {
        question question;
        question.id = q["id"];
        question.title = q["title"];
        question.difficulty = q["difficulty"];
        question.status = q["status"];
        question.paidOnly = q["paidOnly"];
        for (auto& tag : q["topicTags"]) {
            question.topicTags.push_back(tag.value("name", ""));
        }
        questionList.push_back(question);
    }
    return questionList;
}

void printGetAllQuestions() {
    std::vector<question> questionList = getAllQuestions();

    for (auto& q : questionList) {

        std::cout << q.id << std::endl;
        std::cout << q.title << std::endl;
        std::cout << q.difficulty << std::endl;
        std::cout << q.status << std::endl;
        std::cout << q.paidOnly << std::endl;

        for (auto& tag : q.topicTags) {
            std::cout << tag<<" ";
        }

        std::cout<<std::endl << "-------------------" << std::endl;
    }
}





