#ifndef RTSPSERVER_LOG_H
#define RTSPSERVER_LOG_H

#include <bits/types/time_t.h>

#include <string>

#include <time.h>

#include <vector>

static std::string getTime() {
    const char* timeFormat = "%Y-%m-%d %H:%M:%S";
    time_t t = time(nullptr);
    char timeString[64];
    strftime(timeString, sizeof(timeString), timeFormat, localtime(&t));

    return timeString;
}

static std::string getFile(std::string file) {
    std::string pattern = "/";
    std::string::size_type pos;
    std::vector<std::string> result;
    file += pattern;

    int size = file.size();
    for (int i = 0; i < size; ++i) {
        pos = file.find(pattern, i);
        if (pos < size) {
            std::string s = file.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result.back();
}

#define LOGINFO(format, ...)                                                     \
    fprintf(stderr, "[INFO]%s [%s:%d] " format "\n", getTime().data(), __FILE__, \
        __LINE__, ##__VA_ARGS__)
#define LOGERROR(format, ...)                                                     \
    fprintf(stderr, "[ERROR]%s [%s:%d] " format "\n", getTime().data(), __FILE__, \
        __LINE__, ##__VA_ARGS__)

#endif
