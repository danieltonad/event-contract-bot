#include <iostream>
#include "console.h"
#include "database.h"
#include "event.h"
#include "contract.h"


int main() {
    

    // auto console = Console();
    // console.run();

    // 
    initialize_database();
    // int event_id = new_event("test_event", "Chelsea wins Barca?", "2025-12-11 23:59:59", 500'000.0);
    // Event event = get_event_details(std::to_string(event_id));
    
    std::string event_tag = "test_event";
    Event event = get_event_details(event_tag);

    
    // update_order_payouts(1, true);






     // 1. Create a new LMSRContract
    LMSRContract contract(event.id, event.name, event.risk_cap, event.q_yes, event.q_no, event.event_funds);
    
    
    // // 3. Make a few buys/orders
    Order order = contract.buy(Side::YES, 37.0); // Buy $50 on YES
    order = contract.buy(Side::NO, 1010.0);  // Buy $30 on NO
    // order = contract.buy(Side::NO, 10.0);  // Buy $30 on NO
    // order = contract.buy(Side::NO, 160.0);  // Buy $30 on NO
    // order = contract.buy(Side::YES, 1.0);  // Buy $30 on NO
    
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
    // contract.print_state();
    
    auto quote = contract.generate_quote();
    std::cout << "\nGenerated Quote:\n";
    std::cout << "YES Price: " << quote.price_yes << ", NO Price: " << quote.price_no << "\n";
    std::cout << "Size: " << quote.size << "\n";
}