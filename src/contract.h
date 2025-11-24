#pragma once
#include <iostream>
#include "database.h"
#include "orders.h"
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iostream>
#include <iomanip>

class LMSRContract {
public:
    std::string title;
    double risk_cap;
    double b;
    double q_T;
    double q_F;
    std::vector<Order> orders;
    double total_deposits;

    LMSRContract(const std::string& title_, double risk_cap_ = 100.0, double q_T_ = 0.0, double q_F_ = 0.0);

    double cost(double qT, double qF) const;
    std::map<Side,double> price() const;
    Order buy(Side side, double amount);
    double solve_delta_q(Side side, double money) const;

    // optionally metrics and display functions
};
