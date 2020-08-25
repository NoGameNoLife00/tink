#ifndef TINK_STRING_UTIL_H
#define TINK_STRING_UTIL_H

#include <string>

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
}

#endif //TINK_STRING_UTIL_H
