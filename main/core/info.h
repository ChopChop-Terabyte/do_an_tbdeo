#pragma once
#include <string>

struct p_data {
    std::string id;
    std::string name;
    std::string gender;
    std::string age;
    std::string weight;
    std::string height;
};

inline p_data p_info = {"000000", "-NO_DATA", "-NO_DATA", "0", "0", "0.0"};

inline bool upadte = false;
