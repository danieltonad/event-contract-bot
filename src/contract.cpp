#include "contract.h"
#include "database.h" // for new_order, update_event_state
#include "utils.h"    // for new_order, update_event_state
#include <cmath>
#include <iostream>
#include <iomanip>

LMSRContract::LMSRContract(int contract_id_, const std::string &name_, double risk_cap_, double q_T_, double q_F_, double total_deposits_)
    : contract_id(contract_id_), name(name_), risk_cap(risk_cap_), q_T(q_T_), q_F(q_F_), total_deposits(total_deposits_)
{
    b = risk_cap / std::log(2);
}

// ---------------- Cost function ----------------
double LMSRContract::cost(double qT, double qF) const
{
    double m = std::max(qT, qF);
    return b * (std::log(std::exp((qT - m) / b) + std::exp((qF - m) / b)) + m / b);
}

// ---------------- Current price / odds ----------------
std::map<Side, double> LMSRContract::price() const
{
    double m = std::max(q_T, q_F);
    double exp_T = std::exp((q_T - m) / b);
    double exp_F = std::exp((q_F - m) / b);
    double total = exp_T + exp_F;
    return {{Side::YES, exp_T / total}, {Side::NO, exp_F / total}};
}

// ---------------- compute max stake ----------------
double LMSRContract::max_stake(Side side) const {
    double remaining_risk = risk_cap - (cost(q_T, q_F) - cost(0,0));
    if (remaining_risk <= 0.0) return 0.0;

    auto p = price();
    double p_self = (side == Side::YES) ? p[Side::YES] : p[Side::NO];

    // Maximum stake that keeps the market maker within risk_cap
    return remaining_risk * p_self; // approximate linear scaling
}



// ---------------- Trade Execution ----------------
Order LMSRContract::buy(Side side, double stake)
{
    // Compute current max stake allowed for this side
    double current_loss = cost(q_T, q_F) - cost(0, 0);
    double remaining_risk = risk_cap - current_loss;

    if (remaining_risk <= 0.0) {
        warning_msg("Market has reached risk capacity. Order ignored.");
        return Order{}; // no room for trades
    }

    // Compute max stake that would fit without exceeding risk_cap
    auto current_prices = price();
    double p_self = (side == Side::YES) ? current_prices[Side::YES] : current_prices[Side::NO];
    double max_stake_allowed = max_stake(side);  // approximate
    if (stake > max_stake_allowed) {
        warning_msg("Stake: $" + std::to_string(round_figure(stake)) + " exceeds max allowed: $" + std::to_string(round_figure(max_stake_allowed)) + "  for this market. Order ignored.");
        return Order{}; // refuse the order
    }

    // Compute delta_q exactly for two-outcome LMSR
    double delta_q = b * std::log(1 + stake / (b * p_self));

    // Update quantities
    if (side == Side::YES)
        q_T += delta_q;
    else
        q_F += delta_q;

    total_deposits += stake;

    // Recalculate price after update
    current_prices = price();
    double side_price = (side == Side::YES) ? current_prices[Side::YES] : current_prices[Side::NO];

    // Create order object
    Order order{contract_id, stake, round_figure(side_price), round_figure(stake / side_price), side};

    // Persist to database
    new_order(contract_id, (side == Side::YES), stake, order.price, order.expected_cashout);
    update_event_state(contract_id, q_T, q_F, total_deposits);

    return order;
}



// ---------------- Solve delta_q for LMSR ----------------
double LMSRContract::solve_delta_q(Side side, double money) const
{
    double low = 0.0, high = 1e6;
    for (int i = 0; i < 60; ++i)
    {
        double mid = (low + high) / 2;
        double cost_inc = (side == Side::YES) ? cost(q_T + mid, q_F) - cost(q_T, q_F)
                                              : cost(q_T, q_F + mid) - cost(q_T, q_F);
        if (cost_inc < money)
            low = mid;
        else
            high = mid;
    }
    return (low + high) / 2;
}

// ---------------- Display state ----------------
void LMSRContract::print_state() const
{
    auto p = price();
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << "q_T: " << std::fixed << std::setprecision(2) << q_T
              << ", q_F: " << q_F
              << " | price(YES): " << p[Side::YES]
              << ", price(NO): " << p[Side::NO]
              << " |\n total deposits: " << total_deposits << "\n";
    std::cout << "-----------------------------------------------------------------\n";
    std::cout << std::endl;
}