#include "database.h"
#include "utils.h"
#include <ctime>
#include <cstdio>     // for std::sscanf
#include <cctype>     // for std::isdigit

const char* database_path = "database.db";





// Function to initialize the database and create necessary tables
int initialize_database() {
    sqlite3* db;
    

    if (sqlite3_open(database_path, &db)) {
        error_msg("Failed to open database." + std::string(sqlite3_errmsg(db)));
        return 1;
    }

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
            maturity DATETIME NOT NULL,
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
        error_msg("SQL error (events table): " + std::string(errMsg));
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_exec(db, order_book_sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error_msg("SQL error (order_book table): " + std::string(errMsg));
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}












/*************************************************************************
** Event Related Functions
*************************************************************************/
/** create a new event in the events table */
int new_event(const std::string& tag, const std::string& name, const std::string& maturity, double risk_cap) {
    // maturity must follow "YYYY-MM-DD HH:MM:SS"

    if (!valid_maturity(maturity)) {
        error_msg("Invalid maturity format. Expected YYYY-MM-DD HH:MM:SS\n");
        return -1;
    }

    // Parse components and ensure the datetime is not in the past
    int Y, Mo, D, hh, mm, ss;
    if (std::sscanf(maturity.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &Y, &Mo, &D, &hh, &mm, &ss) != 6) {
        error_msg("Invalid maturity contents.\n");
        return -1;
    }

    std::tm tm = {};
    tm.tm_year = Y - 1900;
    tm.tm_mon  = Mo - 1;
    tm.tm_mday = D;
    tm.tm_hour = hh;
    tm.tm_min  = mm;
    tm.tm_sec  = ss;
    tm.tm_isdst = -1; // let mktime determine DST

    time_t maturity_time = std::mktime(&tm);
    if (maturity_time == -1) {
        error_msg("Failed to parse maturity time.\n");
        return -1;
    }

    // mktime normalizes the tm; ensure the normalized values match the input to catch invalid dates (e.g., Feb 30)
    if (tm.tm_year != Y - 1900 || tm.tm_mon != Mo - 1 || tm.tm_mday != D ||
        tm.tm_hour != hh || tm.tm_min != mm || tm.tm_sec != ss) {
        error_msg("Invalid maturity date (out-of-range components).\n");
        return -1;
    }

    time_t now = std::time(nullptr);
    if (maturity_time <= now) {
        error_msg("Maturity must be a future date/time.\n");
        return -1;
    }

    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_open(database_path, &db);
    if (rc != SQLITE_OK) {
        error_msg("Can't open database: " + std::string(db ? sqlite3_errmsg(db) : "unknown"));
        if (db) sqlite3_close(db);
        return -1;
    }

    // Check for existing tag to avoid UNIQUE constraint error
    const char* check_sql = "SELECT id FROM events WHERE tag = ?;";
    rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg("Failed to prepare tag-check statement: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        return -1;
    }
    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int existing_id = sqlite3_column_int(stmt, 0);
        error_msg("Event with tag '" + tag + "' already exists (id=" + to_string_safe(existing_id) + ").");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }
    sqlite3_finalize(stmt);
    stmt = nullptr;

    const char* sql = R"(
        INSERT INTO events (tag, name, risk_cap, maturity)
        VALUES (?, ?, ?, ?)
    )";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        error_msg("Failed to prepare insert statement: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, risk_cap);
    sqlite3_bind_text(stmt, 4, maturity.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    int new_id = -1;
    if (rc != SQLITE_DONE) {
        error_msg("Insert failed: " + std::string(sqlite3_errmsg(db)));
    } else {
        new_id = static_cast<int>(sqlite3_last_insert_rowid(db));
        success_msg("Event added successfully (id=" + to_string_safe(new_id) + ").");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return new_id;
}


// update event outcome and resolve it
void resolve_event_outcome(int event_id, bool outcome) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    // Open DB
    if (sqlite3_open(database_path, &db)) {
        error_msg("Can't open database: " + std::string(sqlite3_errmsg(db)));
        if (db) sqlite3_close(db);
        return;
    }

    // 1. Check if event already resolved
    const char* select_sql = "SELECT resolved, event_funds, q_yes, q_no, win_payout FROM events WHERE id = ?;";
    if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db)));
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
            error_msg("Event already resolved (id=" + to_string_safe(event_id) + ").");
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
    } else {
        error_msg("Event not found (id=" + to_string_safe(event_id) + ").");
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
        error_msg("Failed to prepare update statement: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, outcome ? 1 : 0);
    sqlite3_bind_double(stmt, 2, profit_loss);
    sqlite3_bind_double(stmt, 3, win_payout); // keep existing win_payout
    sqlite3_bind_int(stmt, 4, event_id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        error_msg("Failed to update event: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // 4. Delegate order payout updates
    update_order_payouts(event_id, outcome, win_payout);
}


// update event order counts or other stats
void update_event_state(int event_id, double q_yes, double q_no, double event_funds) {
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(database_path, &db)) {
        error_msg("Can't open database: " + std::string(sqlite3_errmsg(db)));
        return;
    }

    // Round values to 2 decimal places
    q_yes = q_yes;
    q_no = q_no;
    event_funds = round_figure(event_funds);

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
        error_msg("Failed to prepare update statement: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_double(stmt, 1, q_yes);
    sqlite3_bind_double(stmt, 2, q_no);
    sqlite3_bind_double(stmt, 3, event_funds);
    sqlite3_bind_int(stmt, 4, event_id);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        error_msg("Failed to update event state: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


// retrieve event details (for future use)
Event get_event_details(const std::string& id_or_tag) {
    Event ev;
    sqlite3* db = nullptr;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_open(database_path, &db)) {
        error_msg("Can't open database: " + std::string(db ? sqlite3_errmsg(db) : "unknown"));
        if (db) sqlite3_close(db);
        return ev;
    }

    std::string sql = R"(
        SELECT id, tag, name, risk_cap, outcome, resolved, q_yes, q_no, event_funds,
               win_payout, order_count, profit_loss, maturity, created_at, resolved_at
        FROM events
        WHERE
    )";

    bool use_id = is_integer(id_or_tag);
    if (use_id) sql += "id = ?;";
    else sql += "tag = ?;";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        error_msg("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        return ev;
    }

    if (use_id) {
        sqlite3_bind_int(stmt, 1, std::stoi(id_or_tag));
    } else {
        sqlite3_bind_text(stmt, 1, id_or_tag.c_str(), -1, SQLITE_TRANSIENT);
    }

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        ev.id = sqlite3_column_int(stmt, 0);

        const unsigned char* txt = sqlite3_column_text(stmt, 1);
        ev.tag = txt ? reinterpret_cast<const char*>(txt) : std::string();

        txt = sqlite3_column_text(stmt, 2);
        ev.name = txt ? reinterpret_cast<const char*>(txt) : std::string();

        ev.risk_cap = sqlite3_column_double(stmt, 3);

        ev.outcome = (sqlite3_column_type(stmt, 4) == SQLITE_NULL) ? -1 : sqlite3_column_int(stmt, 4);
        ev.resolved = sqlite3_column_int(stmt, 5) != 0;
        ev.q_yes = sqlite3_column_double(stmt, 6);
        ev.q_no = sqlite3_column_double(stmt, 7);
        ev.event_funds = sqlite3_column_double(stmt, 8);
        ev.win_payout = sqlite3_column_double(stmt, 9);
        ev.order_count = sqlite3_column_int(stmt, 10);
        ev.profit_loss = sqlite3_column_double(stmt, 11);

        txt = sqlite3_column_text(stmt, 12);
        ev.maturity = txt ? reinterpret_cast<const char*>(txt) : std::string();

        txt = sqlite3_column_text(stmt, 13);
        ev.created_at = txt ? reinterpret_cast<const char*>(txt) : std::string();

        txt = sqlite3_column_text(stmt, 14);
        ev.resolved_at = txt ? reinterpret_cast<const char*>(txt) : std::string();
    } else if (rc == SQLITE_DONE) {
        error_msg("Event not found for '" + id_or_tag + "'.");
    } else {
        error_msg("Failed to read event: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return ev;
}















/*************************************************************************
** Order Book Related Functions
*************************************************************************/


/** add an order to the order_book table and update aggregate fields on events table */
void new_order(int event_id, bool side, double stake, double price, double expected_cashout) {
    sqlite3* db = nullptr;
    char* errMsg = nullptr;

    if (sqlite3_open(database_path, &db)) {
    error_msg("Can't open database: " + std::string(sqlite3_errmsg(db)));
    if (db) sqlite3_close(db);
    return;
    }

    // Enable foreign key checks
    if (sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error_msg("Failed to enable foreign keys: " + std::string(errMsg ? errMsg : ""));
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }


    // Begin transaction
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error_msg("Failed to begin transaction: " + std::string(errMsg ? errMsg : ""));
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
        error_msg("Failed to prepare insert statement: " + std::string(sqlite3_errmsg(db)));
        success = false;
    } else {
        sqlite3_bind_int(stmt, 1, event_id);
        sqlite3_bind_int(stmt, 2, side ? 1 : 0);
        sqlite3_bind_double(stmt, 3, stake);
        sqlite3_bind_double(stmt, 4, expected_cashout);
        sqlite3_bind_double(stmt, 5, price);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            error_msg("Failed to execute insert statement: " + std::string(sqlite3_errmsg(db)));
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
            error_msg("Failed to prepare update statement: " + std::string(sqlite3_errmsg(db)));
            success = false;
        } else {
            sqlite3_bind_double(stmt, 1, stake);
            sqlite3_bind_int(stmt, 2, event_id);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                error_msg("Failed to execute update statement: " + std::string(sqlite3_errmsg(db)));
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
            error_msg("Failed to commit transaction: " + std::string(errMsg ? errMsg : ""));
            sqlite3_free(errMsg);
        } else {
            success_msg("Order added successfully (event_id=" + to_string_safe(event_id) + ", stake=" + to_string_safe(stake) + ", cashout=" + to_string_safe(expected_cashout) + ", side=" + (side ? "YES" : "NO") + ").");
        }
    } else {
        if (sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            error_msg("Failed to rollback transaction: " + std::string(errMsg ? errMsg : ""));
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
        error_msg("Can't open database: " + std::string(sqlite3_errmsg(db)));
        if (db) sqlite3_close(db);
        return;
    }

    // Begin transaction
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        error_msg("Failed to begin transaction: " + std::string(errMsg ? errMsg : ""));
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
        error_msg("Failed to prepare select statement: " + std::string(sqlite3_errmsg(db)));
        success = false;
    } else {
        sqlite3_bind_int(select_stmt, 1, event_id);

        const char* upd_sql = R"(
            UPDATE order_book
            SET pay_out = ?
            WHERE id = ?;
        )";

        if (sqlite3_prepare_v2(db, upd_sql, -1, &update_stmt, nullptr) != SQLITE_OK) {
            error_msg("Failed to prepare update statement for order_book: " + std::string(sqlite3_errmsg(db)));
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
                    error_msg("Failed to update order payout (id=" + std::to_string(order_id) + "): " + std::string(sqlite3_errmsg(db)));
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
            error_msg("Failed to prepare update statement for events: " + std::string(sqlite3_errmsg(db)));
            success = false;
        } else {
            sqlite3_bind_int(ev_stmt, 1, outcome ? 1 : 0);
            sqlite3_bind_double(ev_stmt, 2, total_payouts);
            sqlite3_bind_double(ev_stmt, 3, total_payouts);
            sqlite3_bind_double(ev_stmt, 4, total_payouts);
            sqlite3_bind_int(ev_stmt, 5, event_id);

            if (sqlite3_step(ev_stmt) != SQLITE_DONE) {
                error_msg("Failed to update event aggregates (id=" + std::to_string(event_id) + "): " + std::string(sqlite3_errmsg(db)));
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
            error_msg("Failed to commit transaction: " + std::string(errMsg ? errMsg : ""));
            sqlite3_free(errMsg);
        } else {
            notify("Order payouts updated for event_id=" + to_string_safe(event_id) + ", total_payouts=" + to_string_safe(total_payouts) + ", Expected total payouts=" + to_string_safe(expected_total_payouts) + ".");
        }
    } else {
        if (sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
            error_msg("Failed to rollback transaction: " + std::string(errMsg ? errMsg : ""));
            sqlite3_free(errMsg);
        }
    }

    sqlite3_close(db);
}








