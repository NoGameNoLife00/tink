#include <string_util.h>
#include <sstream>
#include <vector>

namespace tink::StringUtil {
    void Split(const string &str, char split, std::vector<string> &out) {
        std::stringstream ss(str);
        string s;
        while (getline(ss, s, split))
        {
            out.emplace_back(s);
        }
    }
}