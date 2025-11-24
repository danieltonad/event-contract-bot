#include "database.h"

const char* database_path = "database.db";


// helper function to execute SQL commands without results
// Helper to round a double to 2 decimal places
inline double round2(double value) {
    return std::round(value * 100.0) / 100.0;
}





// Function to initialize the database and create necessary tables
int initialize_database() {
    sqlite3* db;
    

    if (sqlite3_open(database_path, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::cout << "Opened SQLite database successfully!" << std::endl;

    const char* events_sql = R"(
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            tag TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL,
            risk_cap REAL,
            outcome BOOLEAN DEFAULT NULL,
            resolved BOOLEAN DEFAULT 0,
            q_yes REAL DEFAULT 0,
            q_no REAL DEFAULT 0,
            event_funds REAL DEFAULT 0,
            win_payout REAL DEFAULT 0,
            order_count INTEGER DEFAULT 0,
            profit_loss REAL DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            resolved_at DATETIME NULL
        );
    )";

    const char* order_book_sql = R"(
        CREATE TABLE IF NOT EXISTS order_book (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_id INTEGER NOT NULL,
            side INTEGER NOT NULL,
            stake REAL NOT NULL,
            expected_cashout REAL NOT NULL,
            price REAL NOT NULL,
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












/*************************************************************************
** Event Related Functions
*************************************************************************/
/** create a new event in the events table */
void new_event(const std::string& tag, const std::string& name, double risk_cap = 100.0) {
    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc;

    // Open database
    rc = sqlite3_open(database_path, &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // SQL with placeholders
    const char* sql = R"(
        INSERT INTO events (tag, name, risk_cap)
        VALUES (?, ?, ?)
    )";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    // Bind parameters
    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, risk_cap);

    // Execute
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Event added successfully." << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


// update event outcome and resolve it
void resolve_event_outcome(int event_id, bool outcome) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    // Open DB
    if (sqlite3_open(database_path, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return;
    }

    // 1. Check if event already resolved
    const char* select_sql = "SELECT resolved, event_funds, q_yes, q_no, win_payout FROM events WHERE id = ?;";
    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare select statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, event_id);

    int rc = sqlite3_step(stmt);
    double event_funds = 0.0, q_yes = 0.0, q_no = 0.0, win_payout = 0.0;
    int resolved_flag = 0;

    if (rc == SQLITE_ROW) {
        resolved_flag = sqlite3_column_int(stmt, 0);
        event_funds = sqlite3_column_double(stmt, 1);
        q_yes = sqlite3_column_double(stmt, 2);
        q_no = sqlite3_column_double(stmt, 3);
        win_payout = sqlite3_column_double(stmt, 4);

        if (resolved_flag) {
            std::cerr << "Event already resolved (id=" << event_id << ")." << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
    } else {
        std::cerr << "Event not found (id=" << event_id << ")." << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    sqlite3_finalize(stmt);

    // 2. Compute profit/loss: event_funds - winning shares
    double profit_loss = event_funds - (outcome ? q_yes : q_no);

    // 3. Update events table with outcome, profit_loss, resolved, and resolved_at
    const char* update_sql = R"(
        UPDATE events
        SET outcome = ?,
            resolved = 1,
            profit_loss = ?,
            win_payout = ?,
            resolved_at = CURRENT_TIMESTAMP
        WHERE id = ?
    )";

    if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, outcome ? 1 : 0);
    sqlite3_bind_double(stmt, 2, profit_loss);
    sqlite3_bind_double(stmt, 3, win_payout); // keep existing win_payout
    sqlite3_bind_int(stmt, 4, event_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to update event: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // 4. Delegate order payout updates
    update_order_payouts(event_id, outcome, win_payout);
}


// update event order counts or other stats
void update_event_state(int event_id, double q_yes, double q_no) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(database_path, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Round values to 2 decimal places
    q_yes = round2(q_yes);
    q_no = round2(q_no);
    double event_funds = round2(q_yes + q_no);

    // Update event: q_yes, q_no, event_funds, increment order_count
    const char* sql = R"(
        UPDATE events
        SET q_yes = ?,
            q_no = ?,
            event_funds = ?,
            order_count = order_count + 1
        WHERE id = ?
    )";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_double(stmt, 1, q_yes);
    sqlite3_bind_double(stmt, 2, q_no);
    sqlite3_bind_double(stmt, 3, event_funds);
    sqlite3_bind_int(stmt, 4, event_id);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to update event state: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
















/*************************************************************************
** Order Book Related Functions
*************************************************************************/


/** add an order to the order_book table and update aggregate fields on events table */
void add_order(int event_id, bool side, double stake, double price, double expected_cashout) {
    sqlite3* db = nullptr;
    char* errMsg = nullptr;

    if (sqlite3_open(database_path, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return;
    }

    // Begin transaction
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << (errMsg ? errMsg : "") << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    const char* insert_sql = R"(
        INSERT INTO order_book (event_id, side, stake, expected_cashout, price)
        VALUES (?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    bool success = true;

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db) << std::endl;
        success = false;
    } else {
        sqlite3_bind_int(stmt, 1, event_id);
        sqlite3_bind_int(stmt, 2, side ? 1 : 0);
        sqlite3_bind_double(stmt, 3, stake);
        sqlite3_bind_double(stmt, 4, expected_cashout);
        sqlite3_bind_double(stmt, 5, price);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Failed to execute insert statement: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        }
    }

    if (stmt) {
        sqlite3_finalize(stmt);
        stmt = nullptr;
    }

    // If insert succeeded, update aggregate fields on events table
    if (success) {
        const char* update_sql = R"(
            UPDATE events
            SET order_count = COALESCE(order_count, 0) + 1,
                event_funds = COALESCE(event_funds, 0) + ?
            WHERE id = ?;
        )";

        if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        } else {
            sqlite3_bind_double(stmt, 1, stake);
            sqlite3_bind_int(stmt, 2, event_id);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to execute update statement: " << sqlite3_errmsg(db) << std::endl;
                success = false;
            }
        }

        if (stmt) {
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }
    }

    // Commit or rollback
    if (success) {
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to commit transaction: " << (errMsg ? errMsg : "") << std::endl;
            sqlite3_free(errMsg);
        } else {
            std::cout << "Order added successfully (event_id=" << event_id << ", stake=" << stake << ")." << std::endl;
        }
    } else {
        if (sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to rollback transaction: " << (errMsg ? errMsg : "") << std::endl;
            sqlite3_free(errMsg);
        }
    }

    sqlite3_close(db);
} 


// update payout for all orders based on event outcome
void update_order_payouts(int event_id, bool outcome, double& expected_total_payouts) {
    sqlite3* db = nullptr;
    char* errMsg = nullptr;

    if (sqlite3_open(database_path, &db)) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        if (db) sqlite3_close(db);
        return;
    }

    // Begin transaction
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: " << (errMsg ? errMsg : "") << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    const char* select_sql = R"(
        SELECT id, side, expected_cashout
        FROM order_book
        WHERE event_id = ? AND pay_out IS NULL;
    )";

    sqlite3_stmt* select_stmt = nullptr;
    sqlite3_stmt* update_stmt = nullptr;
    bool success = true;
    double total_payouts = 0.0;

    if (sqlite3_prepare_v2(db, select_sql, -1, &select_stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare select statement: " << sqlite3_errmsg(db) << std::endl;
        success = false;
    } else {
        sqlite3_bind_int(select_stmt, 1, event_id);

        const char* upd_sql = R"(
            UPDATE order_book
            SET pay_out = ?
            WHERE id = ?;
        )";

        if (sqlite3_prepare_v2(db, upd_sql, -1, &update_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare update statement for order_book: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        } else {
            // Iterate through matching orders and update pay_out
            while (success && sqlite3_step(select_stmt) == SQLITE_ROW) {
                int order_id = sqlite3_column_int(select_stmt, 0);
                int side_int = sqlite3_column_int(select_stmt, 1);
                double expected_cashout = sqlite3_column_double(select_stmt, 2);

                bool order_side = (side_int != 0);
                double payout = (order_side == outcome) ? expected_cashout : 0.0;
                total_payouts += payout;

                sqlite3_reset(update_stmt);
                sqlite3_clear_bindings(update_stmt);
                sqlite3_bind_double(update_stmt, 1, payout);
                sqlite3_bind_int(update_stmt, 2, order_id);

                if (sqlite3_step(update_stmt) != SQLITE_DONE) {
                    std::cerr << "Failed to update order payout (id=" << order_id << "): " << sqlite3_errmsg(db) << std::endl;
                    success = false;
                    break;
                }
            }
        }
    }

    if (select_stmt) {
        sqlite3_finalize(select_stmt);
        select_stmt = nullptr;
    }
    if (update_stmt) {
        sqlite3_finalize(update_stmt);
        update_stmt = nullptr;
    }

    // Update the events aggregate fields and mark resolved
    if (success) {
        const char* update_event_sql = R"(
            UPDATE events
            SET outcome = ?,
                resolved = 1,
                win_payout = COALESCE(win_payout, 0) + ?,
                event_funds = COALESCE(event_funds, 0) - ?,
                profit_loss = COALESCE(profit_loss, 0) - ?,
                resolved_at = CURRENT_TIMESTAMP
            WHERE id = ?;
        )";

        sqlite3_stmt* ev_stmt = nullptr;
        if (sqlite3_prepare_v2(db, update_event_sql, -1, &ev_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare update statement for events: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        } else {
            sqlite3_bind_int(ev_stmt, 1, outcome ? 1 : 0);
            sqlite3_bind_double(ev_stmt, 2, total_payouts);
            sqlite3_bind_double(ev_stmt, 3, total_payouts);
            sqlite3_bind_double(ev_stmt, 4, total_payouts);
            sqlite3_bind_int(ev_stmt, 5, event_id);

            if (sqlite3_step(ev_stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update event aggregates (id=" << event_id << "): " << sqlite3_errmsg(db) << std::endl;
                success = false;
            }
        }

        if (ev_stmt) {
            sqlite3_finalize(ev_stmt);
            ev_stmt = nullptr;
        }
    }

    // Commit or rollback
    if (success) {
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to commit transaction: " << (errMsg ? errMsg : "") << std::endl;
            sqlite3_free(errMsg);
        } else {
            std::cout << "Order payouts updated for event_id=" << event_id << ", total_payouts=" << total_payouts << " Expected total payouts=" << expected_total_payouts << std::endl;
        }
    } else {
        if (sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to rollback transaction: " << (errMsg ? errMsg : "") << std::endl;
            sqlite3_free(errMsg);
        }
    }

    sqlite3_close(db);
}








