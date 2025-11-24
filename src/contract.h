// contract.h
#pragma once
#include "orders.h"
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iostream>
#include <iomanip>

class LMSRContract {
public:
    int contract_id;
    std::string name;
    double risk_cap;
    double b;
    double q_T;
    double q_F;
    double total_deposits;

    LMSRContract(int contract_id_, const std::string &name_, double risk_cap_ = 100.0, double q_T_ = 0.0, double q_F_ = 0.0, double total_deposits_ = 0.0);

    double cost(double qT, double qF) const;
    std::map<Side,double> price() const;
    Order buy(Side side, double stake);
    double solve_delta_q(Side side, double money) const;
    double max_stake(Side side) const;

    struct Metrics {
        double q_T;
        double q_F;
        std::map<Side,double> current_prices;
        double total_deposits;
        double payout_if_TRUE;
        double payout_if_FALSE;
        double pnl_if_TRUE_wins;
        double pnl_if_FALSE_wins;
        double max_theoretical_loss;
    };

    void print_state() const;
};