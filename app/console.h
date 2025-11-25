#pragma once
#include "database.h"
#include "contract.h"
#include "utils.h"
#include "event.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <functional>
#include <memory>
#include <string>

class Console
{
public:
    void run();
    void print_welcome();
    std::unordered_map<int, std::unique_ptr<LMSRContract>> state;

private:
    bool dispatch(const std::string &cmd);
    bool help();
    auto get_input(const std::string& prompt, std::string& out);
    bool add_event();
    bool list_events();
    bool stake_event(Event& event);
    bool event_quote(Event& event);
    bool event_orders(Event& event);
    bool resolve_event(Event& event);
    bool metrics(const int  event_id);
};
