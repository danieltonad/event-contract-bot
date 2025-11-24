#include "console.h"
#include "database.h"
#include <iostream>
#include <algorithm>
#include <limits>

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
              << "*                                Welcome to the Event Contract Bot!                                 *\n"
              << "*****************************************************************************************************\n";
}

bool Console::dispatch(const std::string &cmd)
{
    std::string lowerCmd = cmd;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (lowerCmd == "help")    return help();
    if (lowerCmd == "quit" || lowerCmd == "q") return false;
    if (lowerCmd == "new")     return newEvent();
    if (lowerCmd == "list")    return listEvents();
    if (lowerCmd == "buy")     return buyEvent();
    if (lowerCmd == "metrics") return metrics();

    std::cout << "Unknown command.\n";
    return true;
}

bool Console::help()
{
    std::cout << "Commands:\n"
              << "  help     — show commands\n"
              << "  quit/q   — exit\n"
              << "  new      — create event\n"
              << "  list     — list events\n"
              << "  buy      — buy/order event Yes/No\n"
              << "  metrics  — show event metrics\n";
    return true;
}

bool Console::newEvent()
{
    std::string name;
    std::cout << "Event name: ";
    std::getline(std::cin, name);
    std::cout << "Created event: " << name << "\n";
    return true;
}

bool Console::listEvents()
{
    std::cout << "Listing events...\n";
    return true;
}

bool Console::buyEvent()
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
