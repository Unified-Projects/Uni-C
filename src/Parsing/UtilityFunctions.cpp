#include <Parsing/UtilityFunctions.h>
#include <unordered_set>
#include <random>
#include <algorithm>

using namespace Parsing;

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, 15);
    static std::uniform_int_distribution<uint32_t> disa(11, 37);

    const char *v = "0123456789abcdefeghijklmnopqrstuvwxyz";
    const size_t len = 32;
    std::string uuid(len, ' ');

    for (size_t i = 0; i < len; ++i) {
        if(i == 0){
            uuid[i] = v[disa(gen)]; // First character must be ascii
            continue;
        }
        uuid[i] = v[dis(gen)];
    }
    return uuid;
}

static std::unordered_set<std::string> __GENERATED__USED__IDs = {};

// Function to generate a unique UUID not present in the usedUUIDs set
std::string GenerateID() {
    std::string newUUID;
    do {
        newUUID = generateUUID();
    } while (__GENERATED__USED__IDs.find(newUUID) != __GENERATED__USED__IDs.end());
    return newUUID;
}

std::string Parsing::EatWhitespace(const std::string& str, int* newLines){
    if(str.size() == 0){
        return "";
    }
    size_t pos = str.find_first_not_of(" \t\n\r\f\v");
    if(pos == std::string::npos){
        // Count number of new lines
        newLines[0] += std::count(str.begin(), str.end(), '\n');
        return "";
    }

    newLines[0] += std::count(str.begin(), str.begin() + pos, '\n');

    return str.substr(pos);
}

std::string Parsing::EatChar(const std::string& str){
    if(str.size() == 0){
        return "";
    }
    return str.substr(1);
}

std::string Parsing::EatWord(const std::string& str, bool i){
    if(str.size() == 0){
        return "";
    }
    size_t pos = 0;
    if(i){
        pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.");
    }
    else{
        pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
    }
    if(pos == std::string::npos){
        return "";
    }
    return str.substr(pos);
}

std::string Parsing::EatUntil(const std::string& str, const char& end, int* newLines){
    if(str.size() == 0){
        return "";
    }
    size_t pos = str.find(end) + 1;
    if(pos == std::string::npos){
        newLines[0] += std::count(str.begin(), str.end(), '\n');
        return "";
    }
    newLines[0] += std::count(str.begin(), str.begin() + pos, '\n');
    return str.substr(pos);
}
std::string Parsing::EatUntil(const std::string& str, std::string end, int* newLines){
    if(str.size() == 0){
        return "";
    }
    size_t pos = str.find(end) + end.size();
    if(pos == std::string::npos){
        newLines[0] += std::count(str.begin(), str.end(), '\n');
        return "";
    }
    newLines[0] += std::count(str.begin(), str.begin() + pos, '\n');
    return str.substr(str.find(end)+end.size());
}

std::string Parsing::GetWord(const std::string& str, bool i){
    if(str.size() == 0){
        return "";
    }
    size_t pos = 0;
    if(i){
        pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.");
    }
    else{
        pos = str.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
    }
    if(pos == std::string::npos){
        return "";
    }
    return str.substr(0, pos);
}
char Parsing::GetChar(const std::string& str){
    if(str.size() == 0){
        return 0;
    }
    return str.c_str()[0];
}
std::string Parsing::GetUntil(const std::string& str, const char& end){
    if(str.size() == 0){
        return "";
    }
    return str.substr(0, str.find(end));
}