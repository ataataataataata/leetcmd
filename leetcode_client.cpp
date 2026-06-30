#include "include/leetcode_client.h"
#include "include/http_client.h"


std::vector<question> getAllQuestions() {
    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=LOL; csrftoken=LOL");
    headers.push_back("x-csrftoken:LOL");
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
        question.titleSlug = q["titleSlug"];
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

void getQuestionDetail(std::string titleSlug) {

    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=LOL; csrftoken=LOL");
    headers.push_back("x-csrftoken:LOL");
    headers.push_back("Referer: https://leetcode.com/problemset/");
    headers.push_back("Origin: https://leetcode.com");

    nlohmann::json j;
    j["operationName"] = "questionDetail";
    j["query"] = "\n    query questionDetail($titleSlug: String!) {\n  languageList {\n    id\n    name\n  }\n  submittableLanguageList {\n    id\n    name\n    verboseName\n  }\n  statusList {\n    id\n    name\n  }\n  questionDiscussionTopic(questionSlug: $titleSlug) {\n    id\n    commentCount\n    topLevelCommentCount\n  }\n  ugcArticleOfficialSolutionArticle(questionSlug: $titleSlug) {\n    uuid\n    chargeType\n    canSee\n    hasVideoArticle\n  }\n  question(titleSlug: $titleSlug) {\n    title\n    titleSlug\n    questionId\n    questionFrontendId\n    questionTitle\n    translatedTitle\n    content\n    translatedContent\n    categoryTitle\n    difficulty\n    stats\n    companyTagStatsV2\n    topicTags {\n      name\n      slug\n      translatedName\n    }\n    positionLevelTags {\n      name\n      nameTranslated\n      slug\n    }\n    similarQuestionList {\n      difficulty\n      titleSlug\n      title\n      translatedTitle\n      isPaidOnly\n    }\n    mysqlSchemas\n    dataSchemas\n    frontendPreviews\n    likes\n    dislikes\n    isPaidOnly\n    status\n    canSeeQuestion\n    enableTestMode\n    metaData\n    enableRunCode\n    enableSubmit\n    enableDebugger\n    envInfo\n    isLiked\n    nextChallenges {\n      difficulty\n      title\n      titleSlug\n      questionFrontendId\n    }\n    libraryUrl\n    adminUrl\n    hints\n    codeSnippets {\n      code\n      lang\n      langSlug\n    }\n    exampleTestcaseList\n    hasFrontendPreview\n    featuredContests {\n      titleSlug\n      title\n    }\n    aiJudgingAvailable\n  }\n}\n    ";
    j["variables"]["titleSlug"]=titleSlug;

    response = httpPost(url, j.dump(), headers);
    nlohmann::json r = nlohmann::json::parse(response);
    std::cout << r;
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





