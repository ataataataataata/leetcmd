#ifndef LEETCMD_LEETCODE_CLIENT_H
#define LEETCMD_LEETCODE_CLIENT_H

#include <nlohmann/json.hpp>
#include <vector>

struct questionAtList {
    int id;
    std::string title;
    std::string titleSlug;
    std::string difficulty;
    std::string status;
    bool paidOnly;
    std::vector<std::string> topicTags;
};

struct languageList {
    int id;
    std::string name;
};

struct codeSnippets {
    std::string code;
    std::string lang;
    std::string langSlug;
};

struct topicTags {
    std::string name;
    std::string slug;
    std::string translatedName;
};

struct statusList {
    int id;
    std::string name;
};

struct submittableLanguageList {
    int id;
    std::string name;
    std::string verboseName;
};

struct ugcArticleOfficialSolutionArticle {
    std::string uuid;
    std::string chargeType;
    bool canSee;
    bool hasVideoArticle;
};

struct question{
    std::string adminUrl;
    boolean aiJudgingAvailable;
    boolean canSeeQuestion;
    std::string categoryTitle;
    codeSnippets codeSnippets;
    std::string companyTagStatsV2;
    std::string content;
    //"dataSchemas": [] skipped
    std::string difficulty;
    int dislikes;
    boolean enableDebugger;
    boolean enableRunCode;
    boolean enableSubmit;
    boolean enableTestMode;
    std::string envInfo;
    std::vector<std::string> exampleTestcaseList;
    //"featuredContests": [] skipped
    //"frontendPreviews": "{}"skipped (string)
    //"hasFrontendPreview": false skipped
    //"hints": [] skipped
    //"isLiked": null skipped
    //"isPaidOnly": false skipped
    //libraryUrl": null skipped
    std::string likes;
    std::string metaData;
    //"mysqlSchemas": [] skipped
    //"nextChallenges": [] skipped
    //positionLevelTags": [] skipped
    std::string questionFrontendId;
    std::string questionId;
    std::string questionTitle;
    //"similarQuestionList": [] skipped
    std::string stats;
    std::string status;
    std::string title;
    std::string titleSlug;
    std::vector<topicTags> topicTags;
    //"translatedContent": null skipped
    //"translatedTitle": null skipped
    //questionDiscussionTopic": null skipped  
    std::vector<statusList> statusList;
    std::vector<submittableLanguageList> submittableLanguageList;
    std::vector<ugcArticleOfficialSolutionArticle> ugcArticleOfficialSolutionArticle;
};

struct questionDetail{
    std::vector<languageList> languageList;
    question question;
};



const std::string url ="https://leetcode.com/graphql";

std::vector<questionAtList> getAllQuestions();
void printGetAllQuestions();
void getQuestionDetail(std::string titleSlug);


#endif //LEETCMD_LEETCODE_CLIENT_H
