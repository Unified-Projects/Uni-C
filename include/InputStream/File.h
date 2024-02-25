#pragma once
#include <string>

namespace InputStream
{
    class FileHandler{
    public:
        std::string filename = "";
        std::string contents;

    public: // Constructors
        FileHandler(const std::string& filename);
        ~FileHandler();
        
        int Close();
    
    protected: // Methods
        void Read();
        void Write(std::string contents);

    public: // Opperators
        operator std::string() const { return contents; }
        operator const char*() const { return contents.c_str(); }
        FileHandler& operator = (const std::string& str) {contents = str; return *this;}
        FileHandler& operator += (const std::string& str) {contents += str; return *this;}
    };
} // namespace InputStream
