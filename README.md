
# Event Contract Execution Bot

A minimal event-market execution engine built on the **Logarithmic Market Scoring Rule (LMSR)**.
Supports multiple binary markets, generates LMSR quotes, executes stakes with slippage, persists state using SQLite, and exposes a thread-safe HTTP API.

---

## Features

* Multiple binary event markets (YES/NO)
* LMSR pricing with natural slippage and cost-based execution
* Hard-capped risk via the LMSR liquidity parameter `b`
* Per-order exposure enforcement using `max_stake()`
* Full state persistence through SQLite
* Deterministic restart with no lost inventory
* Thread-safe HTTP API for quotes, orders, and event listings
* Simple interactive console for testing markets

---

## How LMSR Pricing Works

LMSR cost function:

```
C(q) = b * log( exp(qYes/b) + exp(qNo/b) )
```

Probability quote:

```
P(Yes) = exp(qYes/b) / (exp(qYes/b) + exp(qNo/b))
```

Execution cost for a stake:

```
cost = C(q + Δq) − C(q)
```

*Large trades push prices against the trader naturally.*
Maximum loss is bounded by the funded risk cap.

---

## HTTP API Endpoints


### Get all events

```
GET /events
```

Response:

```json
[
  {
    "id": 1,
    "tag": "event_tag",
    "name": "Event Name",
    "liquidity": 10000.00,
    "orders": 5,
    "maturity": "YYYY-MM-DD HH:MM:SS"
  }
]
```

### Get quote for an event

```
GET /quote/<event_id>
```

Response:

```json
{
  "yes_price": 0.53,
  "no_price": 0.47,
  "max_stake": 230900.24
}
```

### Place an order

```
POST /order/<event_id>
Body: { "stake": 500.0, "side": "yes" }
```

Response:

```json
{
  "event_id": "Event id",
  "side": "yes",
  "stake": 500.00,
  "price": 0.53,
  "expected_cashout": 265.00
}
```

---

## State Persistence

* Every executed stake updates `(qYes, qNo)` in SQLite **before confirmation**.
* Engine reloads last committed state on restart — no inconsistencies.
* Thread-safe access ensures concurrent HTTP requests do not corrupt state.

---
## Console vs API

* **Console (Admin Interface)**

  * Create and resolve events
  * Inspect event metrics, quotes and orders
  * Resolve events and automatically calculate payouts for all orders
  * Intended for administrators or operators

* **HTTP API (User Interface)**

  * Accessed by anyone for real-time quoting and order placement
  * Fetch event lists, current quotes, and individual order placement (stakes)
  * All numeric outputs are rounded for clarity
  * Enables multi-user, low-latency interaction with the markets

---

## Build & Run
Build:

```bash
./build.sh
```

Run:

```bash
./build/event-contract-bot
```

Console commands:

```
  new                     — create event
  list                    — list events
  stake                   — stake/order event Yes/No
  quote <event id/tag>    — get quote for event
  orders <event id/tag>   — get orders for event
  resolve <event id/tag>  — resolve event outcome
  metrics <event id>      — show event metrics
  help                    — show commands
  :q                      — exit
  :b                      — back/cancel
```

## Scalability Notes

* Core LMSR engine is isolated and ready for service deployment
* Exposed HTTP API supports multi-user access out of the box
* Contract objects run under mutex guards for thread-safe concurrency
* Use **PostgreSQL** for relational event and order data, with **Redis** (or similar) for fast, real-time access to each contract’s `qYes`/`qNo` curve state

Supports **low-latency, concurrent execution** in production environments.


