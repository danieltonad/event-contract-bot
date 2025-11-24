#include "contract.h"
#include "database.h" // for new_order, update_event_state
#include <cmath>
#include <iostream>
#include <iomanip>

LMSRContract::LMSRContract(int contract_id_, const std::string &title_, double risk_cap_, double q_T_, double q_F_)
    : contract_id(contract_id_), title(title_), risk_cap(risk_cap_), q_T(q_T_), q_F(q_F_), total_deposits(0.0)
{
    b = risk_cap / std::log(2);
}

// ---------------- Cost function ----------------
double LMSRContract::cost(double qT, double qF) const {
    double m = std::max(qT, qF);
    return b * (std::log(std::exp((qT - m)/b) + std::exp((qF - m)/b)) + m/b);
}

// ---------------- Current price / odds ----------------
std::map<Side,double> LMSRContract::price() const {
    double exp_T = std::exp(q_T/b);
    double exp_F = std::exp(q_F/b);
    double total = exp_T + exp_F;
    return {{Side::YES, exp_T/total}, {Side::NO, exp_F/total}};
}

// ---------------- Trade Execution ----------------
Order LMSRContract::buy(Side side, double stake) {
    double delta_q = solve_delta_q(side, stake);
    auto current_prices = price();
    double side_price = (side == Side::YES) ? current_prices[Side::YES] : current_prices[Side::NO];

    if (side == Side::YES) q_T += delta_q;
    else q_F += delta_q;

    total_deposits += stake;

    // Create order object
    Order order{contract_id, stake, side_price, stake / side_price, side};
    orders.push_back(order);

    // Optional: persist to database
    new_order(contract_id, (side == Side::YES), stake, side_price, stake / side_price);
    update_event_state(contract_id, q_T, q_F);

    return order;
}

// ---------------- Solve delta_q for LMSR ----------------
double LMSRContract::solve_delta_q(Side side, double money) const {
    double low = 0.0, high = 1e6;
    for (int i = 0; i < 60; ++i) {
        double mid = (low + high)/2;
        double cost_inc = (side == Side::YES) ? cost(q_T + mid, q_F) - cost(q_T, q_F)
                                              : cost(q_T, q_F + mid) - cost(q_T, q_F);
        if (cost_inc < money) low = mid;
        else high = mid;
    }
    return (low + high)/2;
}

// ---------------- Metrics / Dashboard ----------------
LMSRContract::Metrics LMSRContract::metrics() const {
    double payout_if_true = q_T;
    double payout_if_false = q_F;
    double worst_loss = b * std::log(2);
    double pnl_if_true = total_deposits - payout_if_true;
    double pnl_if_false = total_deposits - payout_if_false;

    std::vector<std::pair<Side,double>> summary;
    for (auto &o : orders)
        summary.emplace_back(o.side, o.stake);

    return Metrics{q_T, q_F, price(), total_deposits, payout_if_true, payout_if_false,
                   pnl_if_true, pnl_if_false, worst_loss, summary};
}

// ---------------- Display state ----------------
void LMSRContract::print_state() const {
    auto p = price();
    std::cout << "q_T: " << std::fixed << std::setprecision(2) << q_T
              << ", q_F: " << q_F
              << ", price(YES): " << p[Side::YES]
              << ", price(NO): " << p[Side::NO]
              << ", total deposits: " << total_deposits << "\n";

    std::cout << "Orders:\n";
    for (auto &o : orders) {
        std::cout << "  Side: " << ((o.side==Side::YES)?"YES":"NO")
                  << ", stake: " << o.stake
                  << ", price: " << o.price
                  << ", expected cashout: " << o.expected_cashout << "\n";
    }

    std::cout << "Total shares outstanding: YES=" << q_T << ", NO=" << q_F << "\n";
    std::cout << "Max loss bound: " << risk_cap << "\n";
}
