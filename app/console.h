#pragma once
#include <string>

class Console
{
public:
    void run();
    void print_welcome();

private:
    bool dispatch(const std::string &cmd);

    bool help();
    auto get_input(const std::string& prompt, std::string& out);
    bool add_event();
    bool list_events();
    bool stake_event(const std::string& tag);
    bool event_quote(const std::string& tag);
    bool event_orders(const int id);
    bool metrics();
};
