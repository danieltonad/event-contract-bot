#pragma once


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






