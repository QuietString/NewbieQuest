#pragma once
#include <string>
#include <algorithm>
#include <cctype>

namespace qreflect {

    inline std::string trim(std::string s) {
        auto notspace = [](int ch){ return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notspace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notspace).base(), s.end());
        return s;
    }

}
