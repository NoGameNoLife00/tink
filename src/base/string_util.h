#ifndef TINK_STRING_UTIL_H
#define TINK_STRING_UTIL_H

#include <string>
#include <vector>
#include <memory>
namespace tink {
    using std::string;
    typedef std::vector<string> StringList;
    typedef std::shared_ptr<string> StringPtr;
    namespace StringUtil {
        void Split(const string& str, char split, StringList& out);
        void Split(std::string_view str, std::string_view split, StringList& out);
        void IdToHex(string &str, uint32_t id);
        template <typename ...Args>
        string Format(std::string_view format, Args && ...args)
        {
            auto size = std::snprintf(nullptr, 0, format.data(), std::forward<Args>(args)...);
            std::string output(size + 1, '\0');
            std::sprintf(&output[0], format.data(), std::forward<Args>(args)...);
            return output;
        }
    };

}

#endif //TINK_STRING_UTIL_H
