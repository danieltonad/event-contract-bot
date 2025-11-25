// contract.h
#pragma once
#include "orders.h"
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <mutex>


struct Quote {
    double price_yes;     // LMSR YES mid-price
    double price_no;      // LMSR NO mid-price (1 - price_yes)

    double size;         // maximum stake size for either side
};

class LMSRContract {
    private:
        mutable std::mutex contract_mutex; 
        double risk_cap;
        double b;
        double q_T;
        double q_F;
        double total_deposits;
    public:
        int contract_id;
        std::string name;

    
    LMSRContract(int contract_id_, const std::string &name_, double risk_cap_ = 100.0, double q_T_ = 0.0, double q_F_ = 0.0, double total_deposits_ = 0.0);
    
    double cost(double qT, double qF) const;
    std::map<Side,double> price() const;
    Order buy(Side side, double stake);
    double solve_delta_q(Side side, double money) const;
    double max_stake() const;
    
    Quote generate_quote() const;
};





