#pragma once
#include <iostream>
#include "sqlite3.h"
#include "orders.h"
#include "event.h"

extern const char* database_path;

// Function to initialize the database and create necessary tables
int initialize_database();


// event related functions
int new_event(const std::string& tag, const std::string& name, const std::string& maturity, const double risk_cap = 1'000.0);
void update_event_state(int event_id, double q_yes, double q_no, double event_funds);
void resolve_event_outcome(int event_id, bool outcome, double& expected_total_payouts);
Event get_event_details(const std::string& id_or_tag);


// order book related functions
void new_order(int event_id, bool side, double stake, double price, double expected_cashout);
void update_order_payouts(int event_id, bool outcome, double& expected_total_payouts);