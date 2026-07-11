#ifndef LEETCMD_LEETCODE_CLIENT_H
#define LEETCMD_LEETCODE_CLIENT_H

#include <nlohmann/json.hpp>
#include <vector>

struct questionAtList
{
    std::string id;
    std::string title;
    std::string titleSlug;
    std::string difficulty;
    std::string status;
    bool paidOnly;
    std::vector<std::string> topicTags;
};

struct language
{
    int id;
    std::string name;
};

struct codeSnippet
{
    std::string code;
    std::string lang;
    std::string langSlug;
};

struct topicTag
{
    std::string name;
    std::string slug;
    std::string translatedName;
};

struct status
{
    int id;
    std::string name;
};

struct submittableLanguage
{
    int id;
    std::string name;
    std::string verboseName;
};

struct ugcArticleOfficialSolutionArticle
{
    std::string uuid;
    std::string chargeType;
    bool canSee;
    bool hasVideoArticle;
};

struct question
{
    std::string adminUrl;
    bool aiJudgingAvailable;
    bool canSeeQuestion;
    std::string categoryTitle;
    std::vector<codeSnippet> codeSnippets;
    std::string companyTagStatsV2;
    std::string content;
    //"dataSchemas": [] skipped
    std::string difficulty;
    int dislikes;
    bool enableDebugger;
    bool enableRunCode;
    bool enableSubmit;
    bool enableTestMode;
    std::string envInfo;
    std::vector<std::string> exampleTestcaseList;
    //"featuredContests": [] skipped
    //"frontendPreviews": "{}"skipped (string)
    //"hasFrontendPreview": false skipped
    //"hints": [] skipped
    //"isLiked": null skipped
    //"isPaidOnly": false skipped
    // libraryUrl": null skipped
    int likes;
    std::string metaData;
    //"mysqlSchemas": [] skipped
    //"nextChallenges": [] skipped
    // positionLevelTags": [] skipped
    std::string questionFrontendId;
    std::string questionId;
    std::string questionTitle;
    //"similarQuestionList": [] skipped
    std::string stats;
    // std::string status;
    std::string title;
    std::string titleSlug;
    std::vector<topicTag> topicTags;
    //"translatedContent": null skipped
    //"translatedTitle": null skipped
    // questionDiscussionTopic": null skipped
    std::vector<status> statusList;
    std::vector<submittableLanguage> submittableLanguageList;
    std::vector<ugcArticleOfficialSolutionArticle> ugcArticleOfficialSolutionArticlee;
};

struct questionDetail
{
    std::vector<language> languageList;
    question questionn;
    std::vector<std::string> exampleTestcaseList;
};

struct submitResponse
{
    long long submissionId;
};

struct submissionDetail
{
    int statusCode;
    int totalCorrect;
    int totalTestcases;
    std::string runtimeDisplay;
    double runtimePercentile;
    std::string memoryDisplay;
    double memoryPercentile;
    std::string compileError;
    std::string runtimeError;
    std::string lastTestcase;
    std::string codeOutput;
    std::string expectedOutput;
};

struct runResponse
{
    std::string interpretId;
    std::string testCase;
};

struct runDetail
{
    int statusCode;
    std::string lang;
    bool runSuccess;

    std::string compileError;
    std::string fullCompileError;

    std::string statusRuntime;
    int memory;

    std::vector<std::string> codeAnswer;
    std::vector<std::string> codeOutput;
    std::vector<std::string> stdOutputList;

    long long taskFinishTime;
    std::string taskName;

    int totalCorrect;
    int totalTestcases;

    double runtimePercentile;
    std::string statusMemory;
    double memoryPercentile;
    std::string prettyLang;
    std::string submissionId;
    std::string statusMsg;
    std::string state;
};

struct QuestionPage
{
    std::vector<questionAtList> questions;
    bool hasMore;
    int totalLength;
};

struct globalData
{
    int userId;

    bool isSignedIn;
    bool isMockUser;
    bool isPremium;
    bool isVerified;
    bool isAdmin;
    bool isSuperuser;
    bool isTranslator;

    std::string premiumCountryCode;

    std::string username;
    std::string realName;
    std::string avatar;

    std::vector<std::string> permissions;
    std::vector<std::string> completedFeatureGuides;

    int activeSessionId;

    bool checkedInToday;

    long long premiumExpiredAt;

    struct NotificationStatus
    {
        long long lastModified;
        int numUnread;
    } notificationStatus;
};

std::vector<questionAtList> getAllQuestions();
QuestionPage getQuestions(int skip, int limit = 100);
void printGetAllQuestions();
questionDetail getQuestionDetail(std::string titleSlug);
void printGetQuestionDetail(std::string titleSlug);
submitResponse submitCode(std::string titleSlug, std::string code, std::string langSlug, std::string questionId);
submissionDetail getSubmitDetail(long long submissionId, std::string titleSlug);
runResponse runCode(std::string data_input, std::string lang, std::string questionId, std::string typedCode, std::string titleSlug);
runDetail getRunDetail(std::string interpret_id, std::string titleSlug);
QuestionPage searchQuestions(int skip, int limit, std::string questionName);
globalData getglobalData();

#endif // LEETCMD_LEETCODE_CLIENT_H