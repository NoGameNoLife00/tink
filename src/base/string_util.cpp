#include <string_util.h>
#include <sstream>
#include <vector>

namespace tink::StringUtil {
    void Split(const string &str, char split, StringList &out) {
        std::stringstream ss(str);
        string s;
        while (getline(ss, s, split))
        {
            out.emplace_back(s);
        }
    }

    void Split(std::string_view str, std::string_view split, StringList &out) {
        size_t front = 0;
        size_t back = str.find_first_of(split, front);
        while (back != str.npos) {
            out.emplace_back(str.substr(front, back - front));
            front = back + 1;
            back = str.find_first_of(split, front);
        }
        if (back - front > 0) {
            out.emplace_back(str.substr(front, back - front));
        }
    }

    void IdToHex(string &str, uint32_t id) {
        int i;
        str.reserve(10);
        static char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
        str[0] = ':';
        for (i=0;i<8;i++) {
            str[i+1] = hex[(id >> ((7-i) * 4))&0xf];
        }
        str[9] = '\0';
    }

}