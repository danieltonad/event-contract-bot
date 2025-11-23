#include "database.h"



int initialize_database() {
    sqlite3* db;
    const char* filename = "database.db";

    if (sqlite3_open(filename, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::cout << "Opened SQLite database successfully!" << std::endl;

    const char* events_sql = R"(
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag TEXT NOT NULL,
            name TEXT NOT NULL,
            market_depth REAL DEFAULT 100,
            start_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            end_date DATETIME NOT NULL,
            outcome BOOLEAN DEFAULT NULL,
            resolved BOOLEAN DEFAULT 0,
            odds_yes REAL DEFAULT NULL,
            odds_no REAL DEFAULT NULL,
            
        );
    )";

    const char* order_book_sql = R"(
        CREATE TABLE IF NOT EXISTS order_book (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_id INTEGER NOT NULL,
            side BOOLEAN NOT NULL,
            amount REAL NOT NULL,
            odds REAL NOT NULL,
            total_shares REAL NOT NULL,
            pay_out REAL DEFAULT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(event_id) REFERENCES events(id)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, events_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error (events table): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_exec(db, order_book_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error (order_book table): " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    std::cout << "Tables created or verified successfully!" << std::endl;

    sqlite3_close(db);
    return 0;
}




void add_event(const std::string& tag, const std::string& name, const std::string& end_date) {
    // Implementation for adding an event to the database
}

void add_order(int event_id, bool side, double amount, double odds, double total_shares) {
    // Implementation for adding an order to the database
}   