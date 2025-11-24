#pragma once
#include <cmath>
#include <sstream>
#include <iostream>

template <typename T>
std::string to_string_safe(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}


inline bool is_integer(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-' || s[0] == '+') i = 1;
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}


inline double round_figure(double value, int decimals = 2) {
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}


inline void error_msg(const std::string& s) {
    std::cout << "\033[91m" + s + "\033[0m" << std::endl;
}

inline void success_msg(const std::string& s) {
    std::cout << "\033[92m" + s + "\033[0m" << std::endl;
}

inline void warning_msg(const std::string& s) {
    std::cout << "\033[2;33m" + s + "\033[0m" << std::endl;
}

inline bool is_valid_datetime(const std::string& s) {
    if (s.size() != 19) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return false;
        } else if (i == 10) {
            if (s[i] != ' ') return false;
        } else if (i == 13 || i == 16) {
            if (s[i] != ':') return false;
        } else {
            if (!std::isdigit(s[i])) return false;
        }
    }
    int year, month, day, hour, minute, second;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return false;
    }
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    if (hour < 0 || hour > 23) return false;
    if (minute < 0 || minute > 59) return false;
    if (second < 0 || second > 59) return false;
    return true;
}

inline bool valid_maturity(const std::string& s) {
    if (s.size() != 19) return false;
    if (s[4] != '-' || s[7] != '-' || s[10] != ' ' || s[13] != ':' || s[16] != ':') return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) continue;
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}