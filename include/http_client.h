#ifndef LEETCMD_HTTP_CLIENT_H
#define LEETCMD_HTTP_CLIENT_H

#include <curl/curl.h>
#include <iostream>
#include <vector>
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

std::string httpPost(std::string url,std::string body,std::vector<std::string> headers);



#endif //LEETCMD_HTTP_CLIENT_H
