#include <iostream>
#include "console.h"


int main() {
    try {
        Console console;
        console.run();
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "Unknown fatal error.\n";
    }
    std::cout << "Program ended.\n";
    return 0;
}

