#include "console.h"
#include "database.h"
#include "utils.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <functional>

void Console::run()
{
    // initialize db
    initialize_database();

    // command loop
    std::string cmd;
    print_welcome();

    while (true)
    {
        std::cout << "> ";
        if (!std::getline(std::cin, cmd))
            break;
        if (!dispatch(cmd))
            break;
    }
}

void Console::print_welcome()
{
    std::cout << "*****************************************************************************************************\n"
              << "*                                Welcome to the Event Contract Exchange Bot!                        *\n"
              << "*****************************************************************************************************\n";
}

bool Console::dispatch(const std::string &cmd)
{
    std::string lowerCmd = cmd;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    if (lowerCmd == "help")
        return help();
    if (lowerCmd == ":q")
        return false;
    if (lowerCmd == "new")
        return add_event();
    if (lowerCmd == "list")
        return list_events();
    if (lowerCmd == "metrics")
        return metrics();


    std::istringstream iss(cmd);
    std::string command, arg;
    iss >> command >> arg;  // split first word as command, second as argument

    if (command == "quote") {
        return event_quote(arg); 
    } 
    if (command == "stake") {
        return stake_event(arg);  
    }
    if (command == "orders") {
        int event_id;
        try {
            event_id = std::stoi(arg);
            Event ev = get_event_details(arg);
            if (ev.id == 0) {
                return true;
            }
        } catch (...) {
            std::cout << "Invalid event ID.\n";
            return true;
        }
        return event_orders(event_id); 
    }

    std::cout << "Unknown command.\n";
    return true;
}

bool Console::help()
{
    std::cout << "Commands:\n"
              << "  new      — create event\n"
              << "  list     — list events\n"
              << "  quote <event id/tag> — get quote for event\n"
              << "  orders <event id/tag> — get orders for event\n"
              << "  stake      — stake/order event Yes/No\n"
              << "  metrics  — show event metrics\n"
              << "  help     — show commands\n"
              << "  :q   — exit\n"
              << "  :b   — back/cancel\n";
    return true;
}

bool get_input(const std::string &prompt, std::string &out)
{
    std::cout << prompt;
    if (!std::getline(std::cin, out))
        return false; // stream died or EOF
    if (out == ":b")
        return false; // user bailed out
    if (out.empty())
        return false; // invalid empty input
    return true;
}

bool ask_and_validate(
    const std::string &prompt,
    std::string &out,
    std::function<bool(const std::string &)> validator)
{
    while (true)
    {
        std::cout << prompt;
        if (!std::getline(std::cin, out))
            return false;
        if (out == ":b")
            return false;
        if (!validator(out))
        {
            std::cout << "Invalid input. Try again or ':b' to cancel.\n";
            continue;
        }
        return true;
    }
}

bool Console::add_event()
{
    std::string name;
    std::string tag;
    std::string maturity;
    int risk_cap = 10'000;

    if (!ask_and_validate("Event name: ", name, nonEmpty))
        return false;

    if (!ask_and_validate("Event tag (unique tag): ", tag, is_alphanumeric))
        return false;

    if (!ask_and_validate("Event maturity (YYYY-MM-DD HH:MM:SS): ", maturity, maturity_at_least_24h_future))
        return false;

    // risk cap
    while (true){
        std::string input;
        std::cout << "Event risk cap (default " << risk_cap << "): ";
        if (!std::getline(std::cin, input))
            return false;
        if (input == ":b")
            return false;

        if (input.empty())
        {
            // User wants default
            break;
        }

        if (!is_valid_risk_cap(input, risk_cap))
        {
            std::cout << "Risk cap must be an integer >= " << risk_cap << ".\n";
            continue;
        }

        risk_cap = std::stoi(input);
        break;
    }


    std::cout << "Creating event..." << std::endl;
    int event_id = new_event(tag, name, maturity, static_cast<double>(risk_cap));
    std::cout << "Event created with ID: " << event_id << ", Tag: " << tag << std::endl;
    
    return true;

}

bool Console::list_events()
{
    std::cout << "Listing ongoing events...\n";
    std::vector<Event> events = list_all_events(false);
    
    print_table(events, {
        {"ID", [](const Event& e){ return std::to_string(e.id); }},
        {"Tag", [](const Event& e){ return e.tag; }},
        {"Name", [](const Event& e){ return e.name; }},
        {"Liquidity (funds)", [](const Event& e) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << e.event_funds;
            return oss.str();
        }},
        {"Orders (count)", [](const Event& e){ return std::to_string(e.order_count); }},
        {"Maturity Date", [](const Event& e){ return e.maturity; }}
    });
    return true;
}

bool Console::stake_event(const std::string& tag)
{
    int id;
    std::cout << "Event id: ";
    std::cin >> id;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Event " << id << " purchased.\n";
    return true;
}

bool Console::metrics()
{
    std::cout << "Total events: " << "\n";
    return true;
}



bool Console::event_quote(const std::string& tag)
{
    std::cout << "Quote for event '" << tag << "':\n";
    return true;
}

bool Console::event_orders(const int id)
{
    std::cout << "Orders for event '" << id << "':\n";
    std::vector<Order> orders = list_event_orders(id);

    print_table(orders, {
        {"Event ID", [](const Order& o){ return std::to_string(o.event_id); }},
        {"Side", [](const Order& o){ return o.side == Side::YES ? "YES" : "NO"; }},
        {"Stake", [](const Order& o){
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << o.stake;
            return oss.str();
        }},
        {"Price", [](const Order& o){
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << o.price;
            return oss.str();
        }},
        {"Expected Cashout", [](const Order& o){
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << o.expected_cashout;
            return oss.str();
        }}
    });
    return true;
}
