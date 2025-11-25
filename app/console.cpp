#include "console.h"

void Console::run()
{
    // initialize db
    initialize_database();

    // command loop
    std::string cmd;
    print_welcome();

    // resume contracts states
    std::vector<Event> events = list_all_events(false);

    for (auto &e : events)
    {
        state[e.id] = std::make_unique<LMSRContract>(e.id, e.name, e.risk_cap, e.q_yes, e.q_no, e.event_funds);
    }

    warning_msg((std::string("[Resumed ") + to_string_safe(state.size()) + " ongoing contracts states from database.]\n").c_str());

    // usage breif
    std::cout << "Type 'help' for list of commands.\n";

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
    std::cout << "\033[1;38;5;88m"
              << "*****************************************************************************************************\n"
              << "*                                Welcome to the Event Contract Exchange Bot!                        *\n"
              << "*****************************************************************************************************\n"
              << "\033[0m";
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

    std::istringstream iss(lowerCmd);
    std::string command, arg;
    iss >> command >> arg; // split first word as command, second as argument

    if (command == "quote")
    {
        Event event;
        try
        {
            event = get_event_details(arg);
            if (event.id == 0)
            {
                return true;
            }
        }
        catch (...)
        {
            std::cout << "Invalid event ID or tag.\n";
            return true;
        }
        return event_quote(event);
    }
    if (command == "stake")
    {
        Event event;
        try
        {
            event = get_event_details(arg);
            if (event.id == 0)
            {
                return true;
            }
        }
        catch (...)
        {
            std::cout << "Invalid event ID or tag.\n";
            return true;
        }
        return stake_event(event);
    }

    if (command == "orders")
    {
        Event event;
        try
        {
            int event_id = std::stoi(arg);
            event = get_event_details(arg);
            if (event.id == 0)
            {
                return true;
            }
        }
        catch (...)
        {
            std::cout << "Invalid event ID.\n";
            return true;
        }
        return event_orders(event);
    }

    if (command == "resolve")
    {
        Event event;
        try
        {
            event = get_event_details(arg);
            if (event.id == 0)
            {
                return true;
            }
        }
        catch (...)
        {
            std::cout << "Invalid event ID or tag.\n";
            return true;
        }
        return resolve_event(event);
    }

    std::cout << "Unknown command.\n";
    return true;
}

bool Console::help()
{
    system("chcp 65001 > nul");
    std::cout << "Commands:\n"
              << "  new      — create event\n"
              << "  list     — list events\n"
              << "  quote <event id/tag> — get quote for event\n"
              << "  orders <event id/tag> — get orders for event\n"
              << "  resolve <event id/tag> — resolve event outcome\n"
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
    while (true)
    {
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

    print_table(events, {{"ID", [](const Event &e)
                          { return std::to_string(e.id); }},
                         {"Tag", [](const Event &e)
                          { return e.tag; }},
                         {"Name", [](const Event &e)
                          { return e.name; }},
                         {"Liquidity (funds)", [](const Event &e)
                          {
                              std::ostringstream oss;
                              oss << std::fixed << std::setprecision(1) << e.event_funds;
                              return oss.str();
                          }},
                         {"Orders (count)", [](const Event &e)
                          { return std::to_string(e.order_count); }},
                         {"Maturity Date", [](const Event &e)
                          { return e.maturity; }}});
    return true;
}

bool Console::stake_event(Event &event)
{
    std::cout << "You are about to stake for event '" << event.name << "':\n";
    // get quote
    Quote quote = state.at(event.id)->generate_quote();

    // build prompt string
    std::ostringstream prompt;
    prompt << "Choose side [YES (" << round_figure(quote.price_yes)
           << ") / NO (" << round_figure(quote.price_no)
           << ") ]: ";
    std::string side_input;

    bool ok = ask_and_validate(
        prompt.str(),
        side_input,
        [](const std::string &s)
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            return lower == "yes" || lower == "no";
        });

    if (!ok)
        return true; // user cancelled with :b or empty

    // Normalize & convert to enum
    std::transform(side_input.begin(), side_input.end(), side_input.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    Side chosen_side = (side_input == "yes") ? Side::YES : Side::NO;

    // stake amount
    std::string stake_input;
    // build prompt with 2-decimal precision for max stake
    std::ostringstream stake_prompt;
    stake_prompt << "Enter stake amount (max $" << std::fixed << std::setprecision(2) << quote.size << "): ";

    ok = ask_and_validate(
        stake_prompt.str(),
        stake_input,
        [max_stake = quote.size](const std::string &s)
        {
            try
            {
                double val = std::stod(s);
                return val > 0.0 && val <= max_stake;
            }
            catch (...)
            {
                return false;
            }
        });

    if (!ok)
        return true; // user cancelled with :b or empty

    quote = state.at(event.id)->generate_quote(); // refresh quote before confirming
    double stake_amount = std::stod(stake_input);
    double expectded_cashout = stake_amount / (chosen_side == Side::YES ? quote.price_yes : quote.price_no);
    std::cout << "Staking $" << round_figure(stake_amount)
              << " on " << (chosen_side == Side::YES ? "YES" : "NO")
              << " with expected cashout of $" << std::fixed << std::setprecision(2) << expectded_cashout << "\n";

    // confirm and place order
    std::string confirm_input;
    ok = ask_and_validate(
        "Confirm order? [y/n]: ",
        confirm_input,
        [](const std::string &s)
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            return lower == "y" || lower == "n";
        });

    if (!ok || confirm_input == "n")
    {
        std::cout << "Order cancelled.\n";
        return true;
    }
    else
    {
        // place order
        Order order = state.at(event.id)->buy(chosen_side, stake_amount);
        if (order.event_id == 0)
        {
            std::cout << "Order failed.\n";
            return true;
        }

        else
        {
            std::cout << "Order placed successfully: "
                      << "Stake $" << std::fixed << std::setprecision(2) << order.stake
                      << " on " << (order.side == Side::YES ? "YES" : "NO")
                      << " at price " << std::fixed << std::setprecision(2) << order.price
                      << " with expected cashout of $" << std::fixed << std::setprecision(2) << order.expected_cashout
                      << "\n";
        }
    }

    return true;
}

bool Console::event_quote(Event &event)
{
    std::cout << "Quote for event '" << event.name << "':\n";
    Quote quote = state.at(event.id)->generate_quote();
    std::cout << "YES Price: " << std::fixed << std::setprecision(2) << quote.price_yes
              << ", NO Price: " << std::fixed << std::setprecision(2) << quote.price_no
              << ", Max Stake: " << std::fixed << std::setprecision(1) << quote.size << "\n";
    return true;
}

bool Console::event_orders(Event &event)
{
    std::cout << "Orders for event '" << event.name << "':\n";
    std::vector<Order> orders = list_event_orders(event.id);
    auto columns = std::vector<std::pair<std::string, std::function<std::string(const Order &)>>>{
        {"Stake", [](const Order &o)
         {
             std::ostringstream oss;
             oss << std::fixed << std::setprecision(1) << o.stake;
             return oss.str();
         }},
        {"Side", [](const Order &o)
         { return o.side == Side::YES ? "YES" : "NO"; }},
        {"Price", [](const Order &o)
         {
             std::ostringstream oss;
             oss << std::fixed << std::setprecision(2) << o.price;
             return oss.str();
         }},
        {"Expected Cashout", [](const Order &o)
         {
             std::ostringstream oss;
             oss << std::fixed << std::setprecision(2) << o.expected_cashout;
             return oss.str();
         }}};

    // Only add "Payout" column if the event is resolved
    if (event.resolved)
    {
        columns.push_back({"Payout", [](const Order &o) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << o.payout;
            return oss.str();
        }});
    }

    print_table(orders, columns);

    return true;
}

bool Console::resolve_event(Event &event)
{
    std::cout << "Resolving outcome for event '" << event.name << "':\n";

    std::string outcome_input;
    bool ok = ask_and_validate(
        "Enter event outcome [YES/NO]: ",
        outcome_input,
        [](const std::string &s)
        {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c)
                           { return std::tolower(c); });
            return lower == "yes" || lower == "no";
        });

    if (!ok)
        return true; // user cancelled with :b or empty

    // Normalize & convert to bool
    std::transform(outcome_input.begin(), outcome_input.end(), outcome_input.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    bool outcome = (outcome_input == "yes");

    double expected_total_payouts = 0.0;
    resolve_event_outcome(event.id, outcome);

    std::cout << "Event resolved as '" << (outcome ? "YES" : "NO") << "'. "
              << "Expected total payouts: $" << std::fixed << std::setprecision(2) << expected_total_payouts << "\n";

    return true;
}

bool Console::metrics()
{
    std::cout << "Total events: " << "\n";
    return true;
}