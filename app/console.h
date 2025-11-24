#pragma once
#include <string>

class Console
{
public:
    void run();
    void print_welcome();

private:
    bool dispatch(const std::string &cmd);

    bool help();
    bool newEvent();
    bool listEvents();
    bool buyEvent();
    bool metrics();
};
