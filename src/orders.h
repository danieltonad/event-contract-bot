#pragma once
#include <iostream>
#include "sqlite3.h"


// Order Interface

enum class Side {
    YES,
    NO
};

struct Order
{
    int event_id;
    double stake;
    double price;
    double expected_cashout;
    Side side;
};






