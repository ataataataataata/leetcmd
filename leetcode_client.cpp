#include "include/leetcode_client.h"
#include "include/http_client.h"
#include "include/config_manager.h"

std::vector<questionAtList> getAllQuestions()
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
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

QuestionPage getQuestions(int skip, int limit)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers = {
        "Content-Type: application/json",
        token_header,
        csrftoken_header,
        "Referer: https://leetcode.com/problemset/",
        "Origin: https://leetcode.com"};

    const std::string query = R"GQL(
    query problemsetQuestionListV2($filters: QuestionFilterInput, $limit: Int, $searchKeyword: String, $skip: Int, $sortBy: QuestionSortByInput, $categorySlug: String) {
  problemsetQuestionListV2(
    filters: $filters
    limit: $limit
    searchKeyword: $searchKeyword
    skip: $skip
    sortBy: $sortBy
    categorySlug: $categorySlug
  ) {
    questions {
      id
      titleSlug
      title
      translatedTitle
      questionFrontendId
      paidOnly
      difficulty
      topicTags {
        name
        slug
        nameTranslated
      }
      status
      isInMyFavorites
      frequency
      acRate
      contestPoint
    }
    totalLength
    finishedLength
    hasMore
  }
}
    )GQL";

    nlohmann::json body = {
        {"operationName", "problemsetQuestionListV2"},
        {"query", query},
        {"variables", {{"skip", skip}, {"limit", limit}, {"categorySlug", "all-code-essentials"}, {"searchKeyword", ""}, {"sortBy", {{"sortField", "CUSTOM"}, {"sortOrder", "ASCENDING"}}}, {"filters", {{"filterCombineType", "ALL"}, {"statusFilter", {{"questionStatuses", nlohmann::json::array()}, {"operator", "IS"}}}}}}}};

    std::string url = "https://leetcode.com/graphql";
    std::string response = httpPost(url, body.dump(), headers);

    nlohmann::json j = nlohmann::json::parse(response);
    auto &result = j["data"]["problemsetQuestionListV2"];
    auto &questions = result["questions"];

    QuestionPage page;
    page.hasMore = result.value("hasMore", false);
    page.totalLength = result.value("totalLength", 0);

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
            question.topicTags.push_back(tag.value("name", ""));

        page.questions.push_back(question);
    }

    return page;
}

questionDetail getQuestionDetail(std::string titleSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;
    std::string response;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
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

    if (questionJson.contains("exampleTestcaseList") &&
        questionJson["exampleTestcaseList"].is_array())
    {
        for (const auto &tc : questionJson["exampleTestcaseList"])
        {
            q.exampleTestcaseList.push_back(tc.get<std::string>());
        }
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

submitResponse submitCode(std::string titleSlug, std::string code, std::string langSlug, std::string questionId)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;
    std::string response;

    std::string cleanId = "";
    for (char c : questionId)
    {
        if (isdigit(c))
            cleanId += c;
    }

    if (cleanId.empty())
    {
        throw std::runtime_error("CRITICAL: questionId is empty or invalid! Input: " + questionId);
    }

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");

    std::string url = "https://leetcode.com/problems/" + titleSlug + "/submit/";

    nlohmann::json j;
    j["lang"] = langSlug;
    j["question_id"] = std::stoi(questionId);
    j["typed_code"] = code;

    std::string payload = j.dump();

    response = httpPost(url, j.dump(), headers);

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
    // --- Configuration and Header Setup ---
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql";

    // --- Prepare Payload ---
    nlohmann::json j;
    j["operationName"] = "submissionDetails";
    j["query"] = "\n    query submissionDetails($submissionId: Int!) {\n  submissionDetails(submissionId: $submissionId) {\n    runtime\n    runtimeDisplay\n    runtimePercentile\n    runtimeDistribution\n    memory\n    memoryDisplay\n    memoryPercentile\n    memoryDistribution\n    code\n    timestamp\n    statusCode\n    aiJudgeMessage\n    isCompiledLang\n    aiRecheckSubmitted\n    user {\n      username\n      profile {\n        realName\n        userAvatar\n      }\n    }\n    lang {\n      name\n      verboseName\n    }\n    question {\n      questionId\n      titleSlug\n      hasFrontendPreview\n    }\n    notes\n    flagType\n    topicTags {\n      tagId\n      slug\n      name\n    }\n    runtimeError\n    compileError\n    lastTestcase\n    codeOutput\n    expectedOutput\n    totalCorrect\n    totalTestcases\n    fullCodeOutput\n    testDescriptions\n    testBodies\n    testInfo\n    stdOutput\n  }\n}\n    ";
    j["variables"]["submissionId"] = submissionId;

    // --- HTTP Request ---
    std::string payload = j.dump();
    std::string response = httpPost(url, payload, headers);

    nlohmann::json r = nlohmann::json::parse(response);

    // --- Safety Check: Ensure 'data' exists ---
    if (!r.contains("data") || r["data"].is_null() || !r["data"].contains("submissionDetails") || r["data"]["submissionDetails"].is_null())
    {
        submissionDetail sd;
        sd.statusCode = 0; // Treat as pending
        return sd;
    }

    auto &details = r["data"]["submissionDetails"];
    submissionDetail sd;

    // --- Safe Data Extraction ---

    // 1. Status Code
    sd.statusCode = (details.contains("statusCode") && !details["statusCode"].is_null())
                        ? details["statusCode"].get<int>()
                        : 0;

    // 2. Total Correct (With Pending Logic)
    if (details.contains("totalCorrect") && !details["totalCorrect"].is_null())
    {
        sd.totalCorrect = details["totalCorrect"].get<int>();
    }
    else
    {
        sd.totalCorrect = 0;
        sd.statusCode = 0; // If totalCorrect is null, force status 0 to keep polling
    }

    // 3. Other Numeric Fields
    sd.totalTestcases = (details.contains("totalTestcases") && !details["totalTestcases"].is_null())
                            ? details["totalTestcases"].get<int>()
                            : 0;

    sd.runtimePercentile = (details.contains("runtimePercentile") && !details["runtimePercentile"].is_null())
                               ? details["runtimePercentile"].get<double>()
                               : -1.0;

    sd.memoryPercentile = (details.contains("memoryPercentile") && !details["memoryPercentile"].is_null())
                              ? details["memoryPercentile"].get<double>()
                              : -1.0;

    // 4. String Fields
    sd.runtimeDisplay = (details.contains("runtimeDisplay") && !details["runtimeDisplay"].is_null())
                            ? details["runtimeDisplay"].get<std::string>()
                            : "";

    sd.memoryDisplay = (details.contains("memoryDisplay") && !details["memoryDisplay"].is_null())
                           ? details["memoryDisplay"].get<std::string>()
                           : "";

    sd.compileError = (details.contains("compileError") && !details["compileError"].is_null())
                          ? details["compileError"].get<std::string>()
                          : "";

    sd.runtimeError = (details.contains("runtimeError") && !details["runtimeError"].is_null())
                          ? details["runtimeError"].get<std::string>()
                          : "";

    sd.lastTestcase = (details.contains("lastTestcase") && !details["lastTestcase"].is_null())
                          ? details["lastTestcase"].get<std::string>()
                          : "";

    sd.codeOutput = (details.contains("codeOutput") && !details["codeOutput"].is_null())
                        ? details["codeOutput"].get<std::string>()
                        : "";

    sd.expectedOutput = (details.contains("expectedOutput") && !details["expectedOutput"].is_null())
                            ? details["expectedOutput"].get<std::string>()
                            : "";

    return sd;
}

runResponse runCode(std::string data_input, std::string lang, std::string questionId, std::string typedCode, std::string titleSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;
    std::string response;

    std::string cleanId;
    for (char c : questionId)
    {
        if (isdigit(static_cast<unsigned char>(c)))
            cleanId += c;
    }

    if (cleanId.empty())
    {
        throw std::runtime_error("CRITICAL: questionId is empty or invalid! Input: " + questionId);
    }

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");

    std::string url = "https://leetcode.com/problems/" + titleSlug + "/interpret_solution/";

    nlohmann::json j;
    j["data_input"] = data_input;
    j["lang"] = lang;
    j["question_id"] = questionId;
    j["typed_code"] = typedCode;

    response = httpPost(url, j.dump(), headers);

    runResponse rp;

    try
    {
        nlohmann::json r = nlohmann::json::parse(response);

        if (r.contains("interpret_id") && !r["interpret_id"].is_null())
        {
            rp.interpretId = r["interpret_id"].get<std::string>();
        }

        if (r.contains("test_case") && !r["test_case"].is_null())
        {
            rp.testCase = r["test_case"].get<std::string>();
        }

        if (rp.interpretId.empty())
        {
            throw std::runtime_error("interpret_id not found in response: " + response.substr(0, 200));
        }
    }
    catch (const nlohmann::json::parse_error &)
    {
        throw std::runtime_error("Failed to parse run response (likely non-JSON): " + response.substr(0, 200));
    }

    return rp;
}

runDetail getRunDetail(std::string interpret_id, std::string titleSlug)
{

    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Referer: https://leetcode.com/problems/" + titleSlug + "/");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/submissions/detail/" + interpret_id + "/check/";

    std::string response = httpPost(url, "", headers);

    nlohmann::json r = nlohmann::json::parse(response);

    if (!r.contains("status_code") || r["status_code"].is_null())
    {
        runDetail rd;
        rd.statusCode = 0; // Treat as pending
        return rd;
    }

    runDetail rd;

    rd.statusCode = (r.contains("status_code") && !r["status_code"].is_null())
                        ? r["status_code"].get<int>()
                        : 0;

    rd.lang = (r.contains("lang") && !r["lang"].is_null())
                  ? r["lang"].get<std::string>()
                  : "";

    rd.runSuccess = (r.contains("run_success") && !r["run_success"].is_null())
                        ? r["run_success"].get<bool>()
                        : false;

    rd.compileError = (r.contains("compile_error") && !r["compile_error"].is_null())
                          ? r["compile_error"].get<std::string>()
                          : "";

    rd.fullCompileError = (r.contains("full_compile_error") && !r["full_compile_error"].is_null())
                              ? r["full_compile_error"].get<std::string>()
                              : "";

    rd.statusRuntime = (r.contains("status_runtime") && !r["status_runtime"].is_null())
                           ? r["status_runtime"].get<std::string>()
                           : "";

    rd.memory = (r.contains("memory") && !r["memory"].is_null())
                    ? r["memory"].get<int>()
                    : 0;

    if (r.contains("code_answer") && r["code_answer"].is_array())
    {
        rd.codeAnswer = r["code_answer"].get<std::vector<std::string>>();
    }

    if (r.contains("code_output") && r["code_output"].is_array())
    {
        rd.codeOutput = r["code_output"].get<std::vector<std::string>>();
    }

    if (r.contains("std_output_list") && r["std_output_list"].is_array())
    {
        rd.stdOutputList = r["std_output_list"].get<std::vector<std::string>>();
    }

    rd.taskFinishTime = (r.contains("task_finish_time") && !r["task_finish_time"].is_null())
                            ? r["task_finish_time"].get<long long>()
                            : 0;

    rd.taskName = (r.contains("task_name") && !r["task_name"].is_null())
                      ? r["task_name"].get<std::string>()
                      : "";

    rd.totalCorrect = (r.contains("total_correct") && !r["total_correct"].is_null())
                          ? r["total_correct"].get<int>()
                          : 0;

    rd.totalTestcases = (r.contains("total_testcases") && !r["total_testcases"].is_null())
                            ? r["total_testcases"].get<int>()
                            : 0;

    rd.runtimePercentile = (r.contains("runtime_percentile") && !r["runtime_percentile"].is_null())
                               ? r["runtime_percentile"].get<double>()
                               : 0.0;

    rd.statusMemory = (r.contains("status_memory") && !r["status_memory"].is_null())
                          ? r["status_memory"].get<std::string>()
                          : "";

    rd.memoryPercentile = (r.contains("memory_percentile") && !r["memory_percentile"].is_null())
                              ? r["memory_percentile"].get<double>()
                              : 0.0;

    rd.prettyLang = (r.contains("pretty_lang") && !r["pretty_lang"].is_null())
                        ? r["pretty_lang"].get<std::string>()
                        : "";

    rd.submissionId = (r.contains("submission_id") && !r["submission_id"].is_null())
                          ? r["submission_id"].get<std::string>()
                          : "";

    rd.statusMsg = (r.contains("status_msg") && !r["status_msg"].is_null())
                       ? r["status_msg"].get<std::string>()
                       : "";

    rd.state = (r.contains("state") && !r["state"].is_null())
                   ? r["state"].get<std::string>()
                   : "";

    return rd;
}

globalData getglobalData()
{

    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "globalData";
    j["query"] = "\n    query globalData {\n  userStatus {\n    userId\n    isSignedIn\n    isMockUser\n    isPremium\n    premiumCountryCode\n    isVerified\n    username\n    realName\n    avatar\n    isAdmin\n    isSuperuser\n    permissions\n    isTranslator\n    activeSessionId\n    checkedInToday\n    completedFeatureGuides\n    premiumExpiredAt\n    notificationStatus {\n      lastModified\n      numUnread\n    }\n  }\n}\n    ";
    j["variables"] = {};

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    globalData gd;

    if (!r.contains("data") ||
        !r["data"].contains("userStatus") ||
        r["data"]["userStatus"].is_null())
    {
        return gd;
    }

    auto &u = r["data"]["userStatus"];

    gd.userId = (u.contains("userId") && !u["userId"].is_null())
                    ? u["userId"].get<int>()
                    : 0;

    gd.isSignedIn = (u.contains("isSignedIn") && !u["isSignedIn"].is_null())
                        ? u["isSignedIn"].get<bool>()
                        : false;

    gd.isMockUser = (u.contains("isMockUser") && !u["isMockUser"].is_null())
                        ? u["isMockUser"].get<bool>()
                        : false;

    gd.isPremium = (u.contains("isPremium") && !u["isPremium"].is_null())
                       ? u["isPremium"].get<bool>()
                       : false;

    gd.premiumCountryCode = (u.contains("premiumCountryCode") && !u["premiumCountryCode"].is_null())
                                ? u["premiumCountryCode"].get<std::string>()
                                : "";

    gd.isVerified = (u.contains("isVerified") && !u["isVerified"].is_null())
                        ? u["isVerified"].get<bool>()
                        : false;

    gd.username = (u.contains("username") && !u["username"].is_null())
                      ? u["username"].get<std::string>()
                      : "";

    gd.realName = (u.contains("realName") && !u["realName"].is_null())
                      ? u["realName"].get<std::string>()
                      : "";

    gd.avatar = (u.contains("avatar") && !u["avatar"].is_null())
                    ? u["avatar"].get<std::string>()
                    : "";

    gd.isAdmin = (u.contains("isAdmin") && !u["isAdmin"].is_null())
                     ? u["isAdmin"].get<bool>()
                     : false;

    gd.isSuperuser = (u.contains("isSuperuser") && !u["isSuperuser"].is_null())
                         ? u["isSuperuser"].get<bool>()
                         : false;

    gd.isTranslator = (u.contains("isTranslator") && !u["isTranslator"].is_null())
                          ? u["isTranslator"].get<bool>()
                          : false;

    gd.activeSessionId = (u.contains("activeSessionId") && !u["activeSessionId"].is_null())
                             ? u["activeSessionId"].get<int>()
                             : 0;

    gd.checkedInToday = (u.contains("checkedInToday") && !u["checkedInToday"].is_null())
                            ? u["checkedInToday"].get<bool>()
                            : false;

    if (u.contains("permissions") && u["permissions"].is_array())
    {
        gd.permissions = u["permissions"].get<std::vector<std::string>>();
    }

    if (u.contains("completedFeatureGuides") && u["completedFeatureGuides"].is_array())
    {
        gd.completedFeatureGuides =
            u["completedFeatureGuides"].get<std::vector<std::string>>();
    }

    gd.premiumExpiredAt = (u.contains("premiumExpiredAt") && !u["premiumExpiredAt"].is_null())
                              ? u["premiumExpiredAt"].get<long long>()
                              : 0;

    if (u.contains("notificationStatus") &&
        !u["notificationStatus"].is_null())
    {
        auto &n = u["notificationStatus"];

        gd.notificationStatus.lastModified =
            (n.contains("lastModified") && !n["lastModified"].is_null())
                ? n["lastModified"].get<long long>()
                : 0;

        gd.notificationStatus.numUnread =
            (n.contains("numUnread") && !n["numUnread"].is_null())
                ? n["numUnread"].get<int>()
                : 0;
    }

    return gd;
}

QuestionPage searchQuestions(int skip, int limit, std::string questionName)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    std::string query = "\n    query problemsetQuestionListV2($filters: QuestionFilterInput, $limit: Int, $searchKeyword: String, $skip: Int, $sortBy: QuestionSortByInput, $categorySlug: String) {\n  problemsetQuestionListV2(\n    filters: $filters\n    limit: $limit\n    searchKeyword: $searchKeyword\n    skip: $skip\n    sortBy: $sortBy\n    categorySlug: $categorySlug\n  ) {\n    questions {\n      id\n      titleSlug\n      title\n      translatedTitle\n      questionFrontendId\n      paidOnly\n      difficulty\n      topicTags {\n        name\n        slug\n        nameTranslated\n      }\n      status\n      isInMyFavorites\n      frequency\n      acRate\n      contestPoint\n    }\n    totalLength\n    finishedLength\n    hasMore\n  }\n}\n    ";

    nlohmann::json j;
    j["operationName"] = "problemsetQuestionListV2";
    j["query"] = query;
    j["variables"]["searchKeyword"] = questionName;
    j["variables"]["limit"] = limit;
    j["variables"]["skip"] = skip;

    std::string url = "https://leetcode.com/graphql";
    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json resJson = nlohmann::json::parse(response);
    auto &result = resJson["data"]["problemsetQuestionListV2"];
    auto &questions = result["questions"];

    QuestionPage page;
    page.hasMore = result.value("hasMore", false);
    page.totalLength = result.value("totalLength", 0);

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
            question.topicTags.push_back(tag.value("name", ""));

        page.questions.push_back(question);
    }

    return page;
}

FollowData getFollowData(std::string userSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "followCounts";
    j["query"] = "\n    query followCounts($userSlug: String!) {\n  followers(userSlug: $userSlug) {\n    allNum\n  }\n  following(userSlug: $userSlug) {\n    allNum\n  }\n}\n    ";
    j["variables"]["userSlug"] = userSlug;

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    FollowData fd;

    if (!r.contains("data") || r["data"].is_null())
    {
        return fd;
    }

    auto &d = r["data"];

    fd.followers = (d.contains("followers") &&
                    d["followers"].contains("allNum") &&
                    !d["followers"]["allNum"].is_null())
                       ? d["followers"]["allNum"].get<int>()
                       : 0;

    fd.following = (d.contains("following") &&
                    d["following"].contains("allNum") &&
                    !d["following"]["allNum"].is_null())
                       ? d["following"]["allNum"].get<int>()
                       : 0;

    return fd;
}

LanguageProblemData getLanguageStats(std::string userSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "languageStats";
    j["query"] = "\n    query languageStats($username: String!) {\n  matchedUser(username: $username) {\n    languageProblemCount {\n      languageName\n      problemsSolved\n    }\n  }\n}\n    ";
    j["variables"]["username"] = userSlug;

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    LanguageProblemData lpd;

    if (!r.contains("data") ||
        !r["data"].contains("matchedUser") ||
        r["data"]["matchedUser"].is_null())
    {
        return lpd;
    }

    auto &u = r["data"]["matchedUser"];

    if (u.contains("languageProblemCount") &&
        u["languageProblemCount"].is_array())
    {
        for (const auto &item : u["languageProblemCount"])
        {
            LanguageProblem lp;

            lp.languageName = (item.contains("languageName") &&
                               !item["languageName"].is_null())
                                  ? item["languageName"].get<std::string>()
                                  : "";

            lp.problemsSolved = (item.contains("problemsSolved") &&
                                 !item["problemsSolved"].is_null())
                                    ? item["problemsSolved"].get<int>()
                                    : 0;

            lpd.languageProblemCount.push_back(lp);
        }
    }

    return lpd;
}

TagProblemCounts getSkillStats(std::string userSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "skillStats";
    j["query"] = "\n    query skillStats($username: String!) {\n  matchedUser(username: $username) {\n    tagProblemCounts {\n      advanced {\n        tagName\n        tagSlug\n        problemsSolved\n      }\n      intermediate {\n        tagName\n        tagSlug\n        problemsSolved\n      }\n      fundamental {\n        tagName\n        tagSlug\n        problemsSolved\n      }\n    }\n  }\n}\n    ";
    j["variables"]["username"] = userSlug;

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    TagProblemCounts tpc;

    if (!r.contains("data") ||
        !r["data"].contains("matchedUser") ||
        r["data"]["matchedUser"].is_null())
    {
        return tpc;
    }

    auto &u = r["data"]["matchedUser"];

    if (u.contains("tagProblemCounts") &&
        !u["tagProblemCounts"].is_null())
    {
        auto &tags = u["tagProblemCounts"];

        auto parseCategory = [](const nlohmann::json &arr, std::vector<TagProblem> &dest)
        {
            if (!arr.is_array())
                return;

            for (const auto &item : arr)
            {
                TagProblem tp;

                tp.tagName = (item.contains("tagName") &&
                              !item["tagName"].is_null())
                                 ? item["tagName"].get<std::string>()
                                 : "";

                tp.tagSlug = (item.contains("tagSlug") &&
                              !item["tagSlug"].is_null())
                                 ? item["tagSlug"].get<std::string>()
                                 : "";

                tp.problemsSolved = (item.contains("problemsSolved") &&
                                     !item["problemsSolved"].is_null())
                                        ? item["problemsSolved"].get<int>()
                                        : 0;

                dest.push_back(tp);
            }
        };

        if (tags.contains("advanced"))
            parseCategory(tags["advanced"], tpc.advanced);

        if (tags.contains("intermediate"))
            parseCategory(tags["intermediate"], tpc.intermediate);

        if (tags.contains("fundamental"))
            parseCategory(tags["fundamental"], tpc.fundamental);
    }

    return tpc;
}

QuestionProgress getUserProfileUserQuestionProgressV2(std::string userSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "userProfileUserQuestionProgressV2";
    j["query"] = "\n    query userProfileUserQuestionProgressV2($userSlug: String!) {\n  userProfileUserQuestionProgressV2(userSlug: $userSlug) {\n    numAcceptedQuestions {\n      count\n      difficulty\n    }\n    numFailedQuestions {\n      count\n      difficulty\n    }\n    numUntouchedQuestions {\n      count\n      difficulty\n    }\n    userSessionBeatsPercentage {\n      difficulty\n      percentage\n    }\n    totalQuestionBeatsPercentage\n  }\n}\n    ";
    j["variables"]["userSlug"] = userSlug;

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    QuestionProgress qp;

    if (!r.contains("data") ||
        !r["data"].contains("userProfileUserQuestionProgressV2") ||
        r["data"]["userProfileUserQuestionProgressV2"].is_null())
    {
        return qp;
    }

    auto &u = r["data"]["userProfileUserQuestionProgressV2"];

    auto parseQuestionCounts = [](const nlohmann::json &arr, std::vector<QuestionCount> &dest)
    {
        if (!arr.is_array())
            return;

        for (const auto &item : arr)
        {
            QuestionCount qc;

            qc.count = (item.contains("count") &&
                        !item["count"].is_null())
                           ? item["count"].get<int>()
                           : 0;

            qc.difficulty = (item.contains("difficulty") &&
                             !item["difficulty"].is_null())
                                ? item["difficulty"].get<std::string>()
                                : "";

            dest.push_back(qc);
        }
    };

    auto parseBeatPercentages = [](const nlohmann::json &arr, std::vector<QuestionBeatPercentage> &dest)
    {
        if (!arr.is_array())
            return;

        for (const auto &item : arr)
        {
            QuestionBeatPercentage qbp;

            qbp.difficulty = (item.contains("difficulty") &&
                              !item["difficulty"].is_null())
                                 ? item["difficulty"].get<std::string>()
                                 : "";

            qbp.percentage = (item.contains("percentage") &&
                              !item["percentage"].is_null())
                                 ? item["percentage"].get<double>()
                                 : 0.0;

            dest.push_back(qbp);
        }
    };

    if (u.contains("numAcceptedQuestions"))
        parseQuestionCounts(u["numAcceptedQuestions"], qp.numAcceptedQuestions);

    if (u.contains("numFailedQuestions"))
        parseQuestionCounts(u["numFailedQuestions"], qp.numFailedQuestions);

    if (u.contains("numUntouchedQuestions"))
        parseQuestionCounts(u["numUntouchedQuestions"], qp.numUntouchedQuestions);

    if (u.contains("userSessionBeatsPercentage"))
        parseBeatPercentages(u["userSessionBeatsPercentage"], qp.userSessionBeatsPercentage);

    qp.totalQuestionBeatsPercentage =
        (u.contains("totalQuestionBeatsPercentage") &&
         !u["totalQuestionBeatsPercentage"].is_null())
            ? u["totalQuestionBeatsPercentage"].get<double>()
            : 0.0;

    return qp;
}

SubmitStats getuserSessionProgress(std::string userSlug)
{
    std::vector<std::string> tokens = readConfig();
    std::string leetcode_session = tokens[0];
    std::string csrftoken = tokens[1];

    std::string token_header = "Cookie: LEETCODE_SESSION=" + leetcode_session + ";csrftoken=" + csrftoken;
    std::string csrftoken_header = "x-csrftoken: " + csrftoken;

    std::vector<std::string> headers;
    headers.push_back("Content-Type: application/json");
    headers.push_back(token_header);
    headers.push_back(csrftoken_header);
    headers.push_back("User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/150.0.0.0 Safari/537.36");
    headers.push_back("Origin: https://leetcode.com");
    headers.push_back("Accept: */*");
    headers.push_back("Accept-Language: en-US,en;q=0.9");
    headers.push_back("sec-fetch-mode: cors");
    headers.push_back("sec-fetch-site: same-origin");
    headers.push_back("sec-fetch-dest: empty");

    const std::string url = "https://leetcode.com/graphql/";

    nlohmann::json j;
    j["operationName"] = "userSessionProgress";
    j["query"] = "\n    query userSessionProgress($username: String!) {\n  allQuestionsCount {\n    difficulty\n    count\n  }\n  matchedUser(username: $username) {\n    submitStats {\n      acSubmissionNum {\n        difficulty\n        count\n        submissions\n      }\n      totalSubmissionNum {\n        difficulty\n        count\n        submissions\n      }\n    }\n  }\n}\n    ";
    j["variables"]["username"] = userSlug;

    std::string response = httpPost(url, j.dump(), headers);

    nlohmann::json r = nlohmann::json::parse(response);

    SubmitStats ss;

    if (!r.contains("data") || r["data"].is_null())
    {
        return ss;
    }

    auto &d = r["data"];

    auto parseAllQuestions = [](const nlohmann::json &arr, std::vector<AllQuestionsCount> &dest)
    {
        if (!arr.is_array())
            return;

        for (const auto &item : arr)
        {
            AllQuestionsCount aq;

            aq.difficulty = (item.contains("difficulty") &&
                             !item["difficulty"].is_null())
                                ? item["difficulty"].get<std::string>()
                                : "";

            aq.count = (item.contains("count") &&
                        !item["count"].is_null())
                           ? item["count"].get<int>()
                           : 0;

            dest.push_back(aq);
        }
    };

    auto parseSubmissionStats = [](const nlohmann::json &arr, std::vector<SubmissionStat> &dest)
    {
        if (!arr.is_array())
            return;

        for (const auto &item : arr)
        {
            SubmissionStat stat;

            stat.difficulty = (item.contains("difficulty") &&
                               !item["difficulty"].is_null())
                                  ? item["difficulty"].get<std::string>()
                                  : "";

            stat.count = (item.contains("count") &&
                          !item["count"].is_null())
                             ? item["count"].get<int>()
                             : 0;

            stat.submissions = (item.contains("submissions") &&
                                !item["submissions"].is_null())
                                   ? item["submissions"].get<int>()
                                   : 0;

            dest.push_back(stat);
        }
    };

    if (d.contains("allQuestionsCount"))
    {
        parseAllQuestions(d["allQuestionsCount"], ss.allQuestionsCount);
    }

    if (d.contains("matchedUser") &&
        d["matchedUser"].contains("submitStats") &&
        !d["matchedUser"]["submitStats"].is_null())
    {
        auto &submitStats = d["matchedUser"]["submitStats"];

        if (submitStats.contains("acSubmissionNum"))
        {
            parseSubmissionStats(submitStats["acSubmissionNum"], ss.acSubmissionNum);
        }

        if (submitStats.contains("totalSubmissionNum"))
        {
            parseSubmissionStats(submitStats["totalSubmissionNum"], ss.totalSubmissionNum);
        }
    }

    return ss;
}

MePage getMePage(std::string userSlug){
    MePage mePage;
    mePage.followData=getFollowData(userSlug);
    mePage.languageProblemData=getLanguageStats(userSlug);
    mePage.tagProblemCounts=getSkillStats(userSlug);
    mePage.questionProgress=getUserProfileUserQuestionProgressV2(userSlug);
    mePage.submitStats=getuserSessionProgress(userSlug);

    return mePage;
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