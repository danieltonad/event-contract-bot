#pragma once
#include <iostream>
#include "sqlite3.h"
#include "orders.h"

extern const char* database_path;

// Function to initialize the database and create necessary tables
int initialize_database();


// order book related functions
void add_order(int event_id, bool side, double stake, double price, double expected_cashout);
void update_event_state(int event_id, double q_yes, double q_no, double event_funds);
void resolve_event_outcome(int event_id, bool outcome, double& expected_total_payouts);



// event related functions
void update_order_payouts(int event_id, bool outcome, double& expected_total_payouts);