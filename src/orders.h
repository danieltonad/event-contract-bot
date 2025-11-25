#pragma once


// Order Interface

enum class Side {
    NO,
    YES
};

struct Order
{
    int event_id;
    double stake;
    double price;
    double expected_cashout;
    Side side;
    double payout;
};






