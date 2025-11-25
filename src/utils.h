#pragma once
#include <cmath>
#include <sstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <functional>


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




inline bool is_alphanumeric(const std::string& s) {
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c))) return false;
    }
    return true;
}


inline bool is_positive_number(const std::string& s) {
    try {
        double val = std::stod(s);
        return val > 0.0;
    } catch (...) {
        return false;
    }
}

inline bool nonEmpty(const std::string& s) { return !s.empty(); }

inline bool is_valid_risk_cap(const std::string& s, int min_value) {
    try {
        int val = std::stoi(s);
        return val >= min_value;
    } catch (...) {
        return false;
    }
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

inline void notify(const std::string& s) {
    std::cout << "\033[3;34m" + s + "\033[0m" << std::endl;
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
    const bool ok =
        s[4] == '-' && s[7] == '-' &&
        s[10] == ' ' &&
        s[13] == ':' && s[16] == ':';
    if (!ok) return false;

    for (int i : {0,1,2,3,5,6,8,9,11,12,14,15,17,18})
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;

    return true;
}


inline bool maturity_at_least_24h_future(const std::string& s) {
    if (!valid_maturity(s)) return false;

    int Y, M, D, h, m, sec;
    if (std::sscanf(s.c_str(), "%4d-%2d-%2d %2d:%2d:%2d",
                    &Y, &M, &D, &h, &m, &sec) != 6)
        return false;

    std::tm tm_val = {};
    tm_val.tm_year = Y - 1900;
    tm_val.tm_mon  = M - 1;
    tm_val.tm_mday = D;
    tm_val.tm_hour = h;
    tm_val.tm_min  = m;
    tm_val.tm_sec  = sec;
    tm_val.tm_isdst = -1;  // let the system decide

    std::time_t event_time = std::mktime(&tm_val);
    if (event_time == -1) return false;

    std::time_t now = std::time(nullptr);
    std::time_t min_time = now + 24 * 60 * 60;

    return event_time >= min_time;
}









// table
template<typename T>
inline void print_table(const std::vector<T>& items, const std::vector<std::pair<std::string, std::function<std::string(const T&)>>>& columns){
    // Calculate column widths
    std::vector<size_t> widths;
    widths.reserve(columns.size());
    for (const auto& col : columns) {
        size_t max_width = col.first.size();
        for (const auto& item : items) {
            size_t len = col.second(item).size();
            if (len > max_width) max_width = len;
        }
        widths.push_back(max_width);
    }

    // Print header
    std::cout << '+';
    for (size_t w : widths) std::cout << std::string(w + 2, '-') << '+';
    std::cout << '\n';

    std::cout << '|';
    for (size_t i = 0; i < columns.size(); ++i) {
        std::cout << ' ' << std::setw(widths[i]) << std::left << columns[i].first << ' ' << '|';
    }
    std::cout << '\n';

    std::cout << '+';
    for (size_t w : widths) std::cout << std::string(w + 2, '-') << '+';
    std::cout << '\n';

    // Print rows
    for (const auto& item : items) {
        std::cout << '|';
        for (size_t i = 0; i < columns.size(); ++i) {
            std::cout << ' ' << std::setw(widths[i]) << std::left << columns[i].second(item) << ' ' << '|';
        }
        std::cout << '\n';
    }

    std::cout << '+';
    for (size_t w : widths) std::cout << std::string(w + 2, '-') << '+';
    std::cout << '\n';
}
