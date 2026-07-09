#include "include/leetcode_client.h"
#include "include/http_client.h"
#include <fstream>

std::vector<questionAtList> getAllQuestions()
{
    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=SESSION_TOKEN_REDACTED; csrftoken=CSRF_TOKEN_REDACTED");
    headers.push_back("x-csrftoken:CSRF_TOKEN_REDACTED");
    headers.push_back("Referer: https://leetcode.com/problemset/");
    headers.push_back("Origin: https://leetcode.com");

    std::string bodyText = R"JSON({"operationName":"problemsetQuestionListV2","query":"\n    query problemsetQuestionListV2($filters: QuestionFilterInput, $limit: Int, $searchKeyword: String, $skip: Int, $sortBy: QuestionSortByInput, $categorySlug: String) {\n  problemsetQuestionListV2(\n    filters: $filters\n    limit: $limit\n    searchKeyword: $searchKeyword\n    skip: $skip\n    sortBy: $sortBy\n    categorySlug: $categorySlug\n  ) {\n    questions {\n      id\n      titleSlug\n      title\n      translatedTitle\n      questionFrontendId\n      paidOnly\n      difficulty\n      topicTags {\n        name\n        slug\n        nameTranslated\n      }\n      status\n      isInMyFavorites\n      frequency\n      acRate\n      contestPoint\n    }\n    totalLength\n    finishedLength\n    hasMore\n  }\n}\n    ","variables":{"skip":0,"limit":100,"categorySlug":"all-code-essentials","searchKeyword":"","sortBy":{"sortField":"CUSTOM","sortOrder":"ASCENDING"},"filters":{"filterCombineType":"ALL","statusFilter":{"questionStatuses":[],"operator":"IS"}}}})JSON";

    std::string url = "https://leetcode.com/graphql";
    response = httpPost(url, bodyText, headers);

    nlohmann::json j = nlohmann::json::parse(response);

    std::vector<questionAtList> questionList;
    auto questions = j["data"]["problemsetQuestionListV2"]["questions"];
    for (auto &q : questions)
    {
        questionAtList question;
        
        question.id = q["questionFrontendId"].is_string()
                          ? q["questionFrontendId"].get<std::string>()
                          : std::to_string(q["questionFrontendId"].get<int>());
        question.title = q["title"];
        question.titleSlug = q["titleSlug"];
        question.difficulty = q["difficulty"];
        question.status = q["status"];
        question.paidOnly = q["paidOnly"];
        for (auto &tag : q["topicTags"])
        {
            question.topicTags.push_back(tag.value("name", ""));
        }
        questionList.push_back(question);
    }
    return questionList;
}

questionDetail getQuestionDetail(std::string titleSlug)
{

    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=SESSION_TOKEN_REDACTED; csrftoken=CSRF_TOKEN_REDACTED");
    headers.push_back("x-csrftoken:CSRF_TOKEN_REDACTED");
    headers.push_back("Referer: https://leetcode.com/problemset/");
    headers.push_back("Origin: https://leetcode.com");

    nlohmann::json j;
    j["operationName"] = "questionDetail";
    j["query"] = "\n    query questionDetail($titleSlug: String!) {\n  languageList {\n    id\n    name\n  }\n  submittableLanguageList {\n    id\n    name\n    verboseName\n  }\n  statusList {\n    id\n    name\n  }\n  questionDiscussionTopic(questionSlug: $titleSlug) {\n    id\n    commentCount\n    topLevelCommentCount\n  }\n  ugcArticleOfficialSolutionArticle(questionSlug: $titleSlug) {\n    uuid\n    chargeType\n    canSee\n    hasVideoArticle\n  }\n  question(titleSlug: $titleSlug) {\n    title\n    titleSlug\n    questionId\n    questionFrontendId\n    questionTitle\n    translatedTitle\n    content\n    translatedContent\n    categoryTitle\n    difficulty\n    stats\n    companyTagStatsV2\n    topicTags {\n      name\n      slug\n      translatedName\n    }\n    positionLevelTags {\n      name\n      nameTranslated\n      slug\n    }\n    similarQuestionList {\n      difficulty\n      titleSlug\n      title\n      translatedTitle\n      isPaidOnly\n    }\n    mysqlSchemas\n    dataSchemas\n    frontendPreviews\n    likes\n    dislikes\n    isPaidOnly\n    status\n    canSeeQuestion\n    enableTestMode\n    metaData\n    enableRunCode\n    enableSubmit\n    enableDebugger\n    envInfo\n    isLiked\n    nextChallenges {\n      difficulty\n      title\n      titleSlug\n      questionFrontendId\n    }\n    libraryUrl\n    adminUrl\n    hints\n    codeSnippets {\n      code\n      lang\n      langSlug\n    }\n    exampleTestcaseList\n    hasFrontendPreview\n    featuredContests {\n      titleSlug\n      title\n    }\n    aiJudgingAvailable\n  }\n}\n    ";
    j["variables"]["titleSlug"] = titleSlug;

    std::string url = "https://leetcode.com/graphql";

    response = httpPost(url, j.dump(), headers);
    nlohmann::json r = nlohmann::json::parse(response);

    question q;
    auto &questionJson = r["data"]["question"];

    q.adminUrl = (questionJson.contains("adminUrl") && !questionJson["adminUrl"].is_null())
                     ? questionJson["adminUrl"].get<std::string>()
                     : "";

    q.aiJudgingAvailable = questionJson.value("aiJudgingAvailable", false);
    q.canSeeQuestion = questionJson.value("canSeeQuestion", false);

    q.categoryTitle = (questionJson.contains("categoryTitle") && !questionJson["categoryTitle"].is_null())
                          ? questionJson["categoryTitle"].get<std::string>()
                          : "";

    q.companyTagStatsV2 = (questionJson.contains("companyTagStatsV2") && !questionJson["companyTagStatsV2"].is_null())
                              ? questionJson["companyTagStatsV2"].get<std::string>()
                              : "";

    q.content = (questionJson.contains("content") && !questionJson["content"].is_null())
                    ? questionJson["content"].get<std::string>()
                    : "";

    q.difficulty = (questionJson.contains("difficulty") && !questionJson["difficulty"].is_null())
                       ? questionJson["difficulty"].get<std::string>()
                       : "";

    q.envInfo = (questionJson.contains("envInfo") && !questionJson["envInfo"].is_null())
                    ? questionJson["envInfo"].get<std::string>()
                    : "";

    q.metaData = (questionJson.contains("metaData") && !questionJson["metaData"].is_null())
                     ? questionJson["metaData"].get<std::string>()
                     : "";

    q.questionFrontendId = (questionJson.contains("questionFrontendId") && !questionJson["questionFrontendId"].is_null())
                               ? questionJson["questionFrontendId"].get<std::string>()
                               : "";

    q.questionId = (questionJson.contains("questionId") && !questionJson["questionId"].is_null())
                       ? questionJson["questionId"].get<std::string>()
                       : "";

    q.questionTitle = (questionJson.contains("questionTitle") && !questionJson["questionTitle"].is_null())
                          ? questionJson["questionTitle"].get<std::string>()
                          : "";

    q.stats = (questionJson.contains("stats") && !questionJson["stats"].is_null())
                  ? questionJson["stats"].get<std::string>()
                  : "";

    q.title = (questionJson.contains("title") && !questionJson["title"].is_null())
                  ? questionJson["title"].get<std::string>()
                  : "";

    q.titleSlug = (questionJson.contains("titleSlug") && !questionJson["titleSlug"].is_null())
                      ? questionJson["titleSlug"].get<std::string>()
                      : "";

    q.dislikes = questionJson.value("dislikes", 0);
    q.likes = questionJson.value("likes", 0);

    q.enableDebugger = questionJson.value("enableDebugger", false);
    q.enableRunCode = questionJson.value("enableRunCode", false);
    q.enableSubmit = questionJson.value("enableSubmit", false);
    q.enableTestMode = questionJson.value("enableTestMode", false);

    if (questionJson.contains("exampleTestcaseList") && questionJson["exampleTestcaseList"].is_array())
    {
        for (auto &testcase : questionJson["exampleTestcaseList"])
            q.exampleTestcaseList.push_back(testcase.get<std::string>());
    }

    if (questionJson.contains("codeSnippets") && questionJson["codeSnippets"].is_array())
    {
        for (auto &cs : questionJson["codeSnippets"])
        {
            codeSnippet snippet;
            snippet.code = cs.value("code", "");
            snippet.lang = cs.value("lang", "");
            snippet.langSlug = cs.value("langSlug", "");
            q.codeSnippets.push_back(snippet);
        }
    }

    if (questionJson.contains("topicTags") && questionJson["topicTags"].is_array())
    {
        for (auto &tag : questionJson["topicTags"])
        {
            topicTag t;
            t.name = tag.value("name", "");
            t.slug = tag.value("slug", "");

            t.translatedName = (tag.contains("translatedName") && !tag["translatedName"].is_null())
                                   ? tag["translatedName"].get<std::string>()
                                   : "";

            q.topicTags.push_back(t);
        }
    }

    if (r["data"]["statusList"].is_array())
    {
        for (auto &s : r["data"]["statusList"])
        {
            status st;
            st.id = s.value("id", 0);
            st.name = s.value("name", "");
            q.statusList.push_back(st);
        }
    }

    if (r["data"]["submittableLanguageList"].is_array())
    {
        for (auto &lang : r["data"]["submittableLanguageList"])
        {
            submittableLanguage sl;
            sl.id = lang.value("id", 0);
            sl.name = lang.value("name", "");
            sl.verboseName = lang.value("verboseName", "");
            q.submittableLanguageList.push_back(sl);
        }
    }

    if (r["data"]["ugcArticleOfficialSolutionArticle"].is_object() && !r["data"]["ugcArticleOfficialSolutionArticle"].is_null())
    {
        auto article = r["data"]["ugcArticleOfficialSolutionArticle"];

        ugcArticleOfficialSolutionArticle u;
        u.uuid = article.value("uuid", "");
        u.chargeType = article.value("chargeType", "");
        u.canSee = article.value("canSee", false);
        u.hasVideoArticle = article.value("hasVideoArticle", false);

        q.ugcArticleOfficialSolutionArticlee.push_back(u);
    }

    questionDetail qd;

    if (r["data"]["languageList"].is_array())
    {
        for (auto &lang : r["data"]["languageList"])
        {
            language l;
            l.id = lang.value("id", 0);
            l.name = lang.value("name", "");
            qd.languageList.push_back(l);
        }
    }

    qd.questionn = q;

    return qd;
}

void logDebug(const std::string &filename, const std::string &label, const std::string &content)
{
    std::ofstream file(filename, std::ios::app);
    if (file.is_open())
    {
        file << "--- " << label << " ---" << std::endl;
        file << content << std::endl;
        file.close();
    }
}

submitResponse submitCode(std::string titleSlug, std::string code, std::string langSlug, std::string questionId)
{
    std::string response;

    std::string cleanId = "";
    for (char c : questionId)
    {
        if (isdigit(c))
            cleanId += c;
    }

    std::cerr << "DEBUG: Attempting to submit. Raw ID: [" << questionId
              << "], Cleaned ID: [" << cleanId << "]" << std::endl;

    if (cleanId.empty())
    {
        throw std::runtime_error("CRITICAL: questionId is empty or invalid! Input: " + questionId);
    }

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=SESSION_TOKEN_REDACTED ; csrftoken=CSRF_TOKEN_REDACTED");
    headers.push_back("x-csrftoken:CSRF_TOKEN_REDACTED");
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");

    std::string url = "https://leetcode.com/problems/" + titleSlug + "/submit/";

    nlohmann::json j;
    j["lang"] = langSlug;
    j["question_id"] = std::stoi(questionId);
    j["typed_code"] = code;

    std::string payload = j.dump();
    logDebug("debug_submit.log", "SUBMIT PAYLOAD", payload);

    response = httpPost(url, j.dump(), headers);
    logDebug("debug_submit.log", "SUBMIT RESPONSE", response);

    submitResponse sr;
    try
    {
        nlohmann::json r = nlohmann::json::parse(response);

        if (r.contains("submission_id") && !r["submission_id"].is_null())
        {
            sr.submissionId = r["submission_id"].get<long long>();
        }
        else
        {
            throw std::runtime_error("Submission ID not found in response: " + response.substr(0, 200));
        }
    }
    catch (const nlohmann::json::parse_error &)
    {
        throw std::runtime_error("Failed to parse submit response (likely non-JSON): " + response.substr(0, 200));
    }

    return sr;
}

submissionDetail getSubmitDetail(long long submissionId, std::string titleSlug)
{
    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back("Cookie: LEETCODE_SESSION=SESSION_TOKEN_REDACTED; csrftoken=CSRF_TOKEN_REDACTED");
    headers.push_back("x-csrftoken:CSRF_TOKEN_REDACTED");
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql";

    nlohmann::json j;
    j["operationName"] = "submissionDetails";
    j["query"] = "\n    query submissionDetails($submissionId: Int!) {\n  submissionDetails(submissionId: $submissionId) {\n    runtime\n    runtimeDisplay\n    runtimePercentile\n    runtimeDistribution\n    memory\n    memoryDisplay\n    memoryPercentile\n    memoryDistribution\n    code\n    timestamp\n    statusCode\n    aiJudgeMessage\n    isCompiledLang\n    aiRecheckSubmitted\n    user {\n      username\n      profile {\n        realName\n        userAvatar\n      }\n    }\n    lang {\n      name\n      verboseName\n    }\n    question {\n      questionId\n      titleSlug\n      hasFrontendPreview\n    }\n    notes\n    flagType\n    topicTags {\n      tagId\n      slug\n      name\n    }\n    runtimeError\n    compileError\n    lastTestcase\n    codeOutput\n    expectedOutput\n    totalCorrect\n    totalTestcases\n    fullCodeOutput\n    testDescriptions\n    testBodies\n    testInfo\n    stdOutput\n  }\n}\n    ";
    j["variables"]["submissionId"] = submissionId;

    std::string payload = j.dump();
    response = httpPost(url, payload, headers);

    logDebug("debug_detail.log", "DETAIL PAYLOAD", payload);
    logDebug("debug_detail.log", "DETAIL RESPONSE", response);

    nlohmann::json r = nlohmann::json::parse(response);

    if (!r.contains("data") || r["data"].is_null())
    {
        throw std::runtime_error("GraphQL error: 'data' field missing in response.");
    }

    submissionDetail sd;
    auto &submissionDetails = r["data"]["submissionDetails"];

    sd.statusCode = (submissionDetails.contains("statusCode") && !submissionDetails["statusCode"].is_null())
                        ? submissionDetails["statusCode"].get<int>()
                        : 0;

    sd.totalCorrect = (submissionDetails.contains("totalCorrect") && !submissionDetails["totalCorrect"].is_null())
                          ? submissionDetails["totalCorrect"].get<int>()
                          : 0;

    sd.totalTestcases = (submissionDetails.contains("totalTestcases") && !submissionDetails["totalTestcases"].is_null())
                            ? submissionDetails["totalTestcases"].get<int>()
                            : 0;

    sd.runtimeDisplay = (submissionDetails.contains("runtimeDisplay") && !submissionDetails["runtimeDisplay"].is_null())
                            ? submissionDetails["runtimeDisplay"].get<std::string>()
                            : "";

    sd.runtimePercentile = (submissionDetails.contains("runtimePercentile") && !submissionDetails["runtimePercentile"].is_null())
                               ? submissionDetails["runtimePercentile"].get<double>()
                               : -1.0;

    sd.memoryDisplay = (submissionDetails.contains("memoryDisplay") && !submissionDetails["memoryDisplay"].is_null())
                           ? submissionDetails["memoryDisplay"].get<std::string>()
                           : "";

    sd.memoryPercentile = (submissionDetails.contains("memoryPercentile") && !submissionDetails["memoryPercentile"].is_null())
                              ? submissionDetails["memoryPercentile"].get<double>()
                              : -1.0;

    sd.compileError = (submissionDetails.contains("compileError") && !submissionDetails["compileError"].is_null())
                          ? submissionDetails["compileError"].get<std::string>()
                          : "";

    sd.runtimeError = (submissionDetails.contains("runtimeError") && !submissionDetails["runtimeError"].is_null())
                          ? submissionDetails["runtimeError"].get<std::string>()
                          : "";

    sd.lastTestcase = (submissionDetails.contains("lastTestcase") && !submissionDetails["lastTestcase"].is_null())
                          ? submissionDetails["lastTestcase"].get<std::string>()
                          : "";

    sd.codeOutput = (submissionDetails.contains("codeOutput") && !submissionDetails["codeOutput"].is_null())
                        ? submissionDetails["codeOutput"].get<std::string>()
                        : "";

    sd.expectedOutput = (submissionDetails.contains("expectedOutput") && !submissionDetails["expectedOutput"].is_null())
                            ? submissionDetails["expectedOutput"].get<std::string>()
                            : "";

    return sd;
}

void printGetAllQuestions()
{
    std::vector<questionAtList> questionList = getAllQuestions();

    for (auto &q : questionList)
    {

        std::cout << q.id << std::endl;
        std::cout << q.title << std::endl;
        std::cout << q.difficulty << std::endl;
        std::cout << q.status << std::endl;
        std::cout << q.paidOnly << std::endl;

        for (auto &tag : q.topicTags)
        {
            std::cout << tag << " ";
        }

        std::cout << std::endl
                  << "-------------------" << std::endl;
    }
}

void printGetQuestionDetail(std::string titleSlug)
{
    questionDetail qd = getQuestionDetail(titleSlug);
    question q = qd.questionn;

    std::cout << "Title: " << q.title << std::endl;
    std::cout << "Difficulty: " << q.difficulty << std::endl;
    std::cout << "Content: " << q.content << std::endl;
    std::cout << "Likes: " << q.likes << std::endl;
    std::cout << "Dislikes: " << q.dislikes << std::endl;

    std::cout << "Topic Tags: ";
    for (auto &tag : q.topicTags)
    {
        std::cout << tag.name << " ";
    }
    std::cout << std::endl;

    std::cout << "Submittable Languages: ";
    for (auto &lang : q.submittableLanguageList)
    {
        std::cout << lang.name << " ";
    }
    std::cout << std::endl;

    std::cout << "UGC Article Official Solution Articles: ";
    for (auto &article : q.ugcArticleOfficialSolutionArticlee)
    {
        std::cout << article.uuid << " ";
    }
    std::cout << std::endl;
}