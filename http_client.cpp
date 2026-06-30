#include "include/http_client.h"


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}


std::string httpPost(std::string url,std::string body,std::vector<std::string> headers) {

    std::string response;

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "curl_easy_init() failed." << std::endl;
    }

    struct curl_slist* slist = NULL;
    for (auto& header : headers) {
        slist = curl_slist_append(slist, header.c_str());
    }

    curl_easy_setopt(curl,CURLOPT_URL,url.c_str());
    curl_easy_setopt(curl,CURLOPT_POST,1L);
    curl_easy_setopt(curl,CURLOPT_POSTFIELDS,body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,slist);

    curl_easy_perform(curl);

    curl_slist_free_all(slist);
    curl_easy_cleanup(curl);

    return response;
}
