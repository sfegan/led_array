#pragma once

#include <string>

struct BuildDate {
    BuildDate(const char* date, const char* time);
    static std::string latest_build_date;
};