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
    return b * (m/b + std::log(std::exp((qT-m)/b) + std::exp((qF-m)/b)));
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
double LMSRContract::max_stake() const
{
    // remaining risk
    double remaining_risk = risk_cap - (cost(q_T, q_F) - cost(0, 0));
    if (remaining_risk <= 0)
        return 0.0;

    // binary search to find dq such that adding dq shares consumes remaining risk
    double low = 0.0;
    double high = 1.0;
    double base_cost = cost(q_T, q_F);

    // expand high until cost increase exceeds remaining risk
    for (int i = 0; i < 60; ++i) {
        double new_cost = cost(q_T + high, q_F + high); // both sides incremented
        if (new_cost - base_cost >= remaining_risk)
            break;
        high *= 2.0;
    }

    // binary search
    for (int i = 0; i < 60; ++i) {
        double mid = 0.5 * (low + high);
        double new_cost = cost(q_T + mid, q_F + mid);
        if (new_cost - base_cost < remaining_risk)
            low = mid;
        else
            high = mid;
    }

    double dq = 0.5 * (low + high);

    // use current LMSR price (mid-point probability) for stake calculation
    auto p = price();
    double p_self = p.at(Side::YES); // could also use NO, same effect

    double max_stake = b * p_self * (std::exp(dq / b) - 1.0);
    return max_stake;
}





// ---------------- Trade Execution ----------------
Order LMSRContract::buy(Side side, double stake)
{
    // ensure thread safety
    std::lock_guard<std::mutex> guard(contract_mutex); 

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
    double max_stake_allowed = max_stake();  // approximate
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



// ---------------- pull realtime quote ----------------
Quote LMSRContract::generate_quote() const {
    // ensure thread safety
    std::lock_guard<std::mutex> guard(contract_mutex); 

    auto p = price(); // current LMSR prices

    // YES and NO prices from LMSR
    double yes_price = p.at(Side::YES);
    double no_price  = p.at(Side::NO);

    // Maximum trade size based on remaining risk
    double size = max_stake();

    return Quote{yes_price, no_price, size};

}






// ---------------- Solve delta_q for LMSR ----------------
double LMSRContract::solve_delta_q(Side side, double money) const
{
    double low = 0.0;
    double high = 1.0;

    // 1. Expand high bound until cost_inc exceeds money
    for (int i = 0; i < 60; ++i)
    {
        double cost_inc =
            (side == Side::YES)
                ? cost(q_T + high, q_F) - cost(q_T, q_F)
                : cost(q_T, q_F + high) - cost(q_T, q_F);

        if (cost_inc >= money)
            break;     // high is now big enough
        high *= 2;     // expand exponentially
    }

    // 2. Binary search
    for (int i = 0; i < 60; ++i)
    {
        double mid = 0.5 * (low + high);
        double cost_inc =
            (side == Side::YES)
                ? cost(q_T + mid, q_F) - cost(q_T, q_F)
                : cost(q_T, q_F + mid) - cost(q_T, q_F);

        if (cost_inc < money)
            low = mid;
        else
            high = mid;
    }

    return 0.5 * (low + high);
}
