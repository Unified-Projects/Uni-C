#include <InputStream/Arguments.h>

using namespace InputStream;

ArgumentParser::ArgumentParser(int argc, char** argv) : argc(argc), argv(argv) {
    for(int i = 0; i < argc; i++){
        tokens.push_back(std::string(argv[i]));
    }
}

bool ArgumentParser::hasToken(const std::string& token){
    return std::find(this->tokens.begin(), this->tokens.end(), token)
            != this->tokens.end();
}

const std::string& ArgumentParser::getTokenValue(const std::string& token){
    std::vector<std::string>::const_iterator itr;
    itr =  std::find(this->tokens.begin(), this->tokens.end(), token);
    if (itr != this->tokens.end() && ++itr != this->tokens.end()){
        return *itr;
    }
    static const std::string empty_string("");
    return empty_string;
}

const std::string& ArgumentParser::getToken(const int& tokenIndex){
    if(tokenIndex < 0 || tokenIndex >= this->tokens.size()){
        static const std::string empty_string("");
        return empty_string;
    }
    return this->tokens[tokenIndex];

}

const int ArgumentParser::getTokenIndex(const std::string& token){
    auto itr =  std::find(this->tokens.begin(), this->tokens.end(), token);
    if (itr != this->tokens.end()){
        return std::distance(this->tokens.begin(), itr);
    }
    return -1;
}

const int ArgumentParser::getTokenType(const std::string& token){
    auto itr =  std::find(this->tokens.begin(), this->tokens.end(), token);
    if (itr != this->tokens.end()){
        if(itr->find('=') != std::string::npos){
            return 2;
        }
        if(itr->c_str()[0] == '-'){
            return 3;
        }
        return 1;
    }
    return 0;
}

const int ArgumentParser::getTokenType(const int& tokenIndex){
    if(tokenIndex < 0 || tokenIndex >= this->tokens.size()){
        return 0;
    }
    auto itr =  this->tokens.begin() + tokenIndex;
    if (itr != this->tokens.end()){
        if(itr->find('=') != std::string::npos){
            return 2;
        }
        if(itr->c_str()[0] == '-'){
            return 3;
        }
        return 1;
    }
    return 0;
}