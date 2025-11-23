#include "database.h"



int initializeDatabase() {
    sqlite3* db;
    const char* filename = "my_database.db";

    if (sqlite3_open(filename, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::cout << "Opened SQLite database successfully!" << std::endl;

    sqlite3_close(db);
    return 0;
}