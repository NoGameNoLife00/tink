#ifndef TINK_STRING_UTIL_H
#define TINK_STRING_UTIL_H

#include <string>
#include <vector>
namespace tink {
    using std::string;
    class StringArg // copyable
    {
    public:
        StringArg(const char* str)
                : str_(str)
        { }

        StringArg(const string& str)
                : str_(str.c_str())
        { }

        const char* c_str() const { return str_; }

    private:
        const char* str_;
    };
    typedef std::vector<string> StringList;
    namespace StringUtil {
        void Split(const string& str, char split, StringList& out);
    };

}

#endif //TINK_STRING_UTIL_H
