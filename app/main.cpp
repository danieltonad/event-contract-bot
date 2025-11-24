#include <iostream>
#include "database.h"
#include "contract.h"
#include <string>

int main() {
    std::cout << "Hello, Event Contract Bot!" << std::endl;

    initialize_database();

    // 
    int event_id = new_event("test_event", "Chelsea wins Barca?", "2025-12-11 23:59:59", 100.0);
    // update_order_payouts(1, true);










     // 1. Create a new LMSRContract
    LMSRContract contract(event_id, "Chelsea wins Barca?", 100.0);

    // 2. Check initial state
    contract.print_state();

    // // 3. Make a few buys/orders
    // Order order1 = contract.buy(Side::YES, 50.0); // Buy $50 on YES
    // Order order2 = contract.buy(Side::NO, 30.0);  // Buy $30 on NO

    // // 4. Print orders and state
    // std::cout << "\nAfter orders:\n";
    // contract.print_state();

    // // 5. Access metrics
    // LMSRContract::Metrics m = contract.metrics();
    // std::cout << "\nMetrics:\n";
    // std::cout << "Payout if YES wins: " << m.payout_if_TRUE << "\n";
    // std::cout << "Payout if NO wins: " << m.payout_if_FALSE << "\n";
    // std::cout << "PNL if YES wins: " << m.pnl_if_TRUE_wins << "\n";
    // std::cout << "PNL if NO wins: " << m.pnl_if_FALSE_wins << "\n";




    return 0;
}