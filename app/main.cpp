#include <iostream>
#include "database.h"

int main() {
    std::cout << "Hello, Event Contract Bot!" << std::endl;

    initialize_database();

    // 
    // add_order(1, false, 34.0, 0.45, 75.56);
    update_order_payouts(1, true);



    return 0;
}