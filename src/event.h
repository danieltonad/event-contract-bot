#pragma once
#include <string>
#include <optional>


struct  Event{
    int id;
    std::string tag;
    std::string name;
    double risk_cap;
    std::optional<bool> outcome;
    bool resolved;
    double q_yes;
    double q_no;
    double event_funds;
    double win_payout;
    int order_count;
    double profit_loss;
    std::string maturity;
    std::string created_at; 
    std::optional<std::string> resolved_at;
};
