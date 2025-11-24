#include <iostream>
#include "database.h"
#include "contract.h"
#include <string>

int main() {
    std::cout << "Hello, Event Contract Bot!" << std::endl;

    initialize_database();

    // 
    // int event_id = new_event("test_event", "Chelsea wins Barca?", "2025-12-11 23:59:59", 100.0);
    // Event event = get_event_details(std::to_string(event_id));
    
    std::string event_tag = "test_event";
    Event event = get_event_details(event_tag);
    
    std::cout << event.name << " " << event.tag << std::endl;
    
    // update_order_payouts(1, true);






     // 1. Create a new LMSRContract
    LMSRContract contract(event.id, event.name, event.resolved ? event.risk_cap : 100.0, event.q_yes, event.q_no, event.event_funds);

    
    // // 3. Make a few buys/orders
    Order order1 = contract.buy(Side::YES, 45.0); // Buy $50 on YES
    Order order2 = contract.buy(Side::NO, 10.0);  // Buy $30 on NO
    
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
    
    
    
    // 2. Check initial state
    contract.print_state();

    return 0;
}