#include <InputStream/File.h>

#include <cstdio>

using namespace InputStream;

FileHandler::FileHandler(const std::string& filename)
    : filename(filename)
{
    Read();
}

FileHandler::~FileHandler(){
    // Nothing Needed Here
}

int FileHandler::Close(){
    Write(contents);
    return 0;
}

void FileHandler::Read(){
    FILE* file = fopen(filename.c_str(), "r");
    if(file == NULL){
        return;
    }
    char buffer[256];
    while(fgets(buffer, 256, file) != NULL){
        contents += buffer;
    }
    fclose(file);
}
void FileHandler::Write(std::string contents){
    FILE* file = fopen(filename.c_str(), "w");
    if(file == NULL){
        return;
    }
    fputs(contents.c_str(), file);
    fclose(file);
}