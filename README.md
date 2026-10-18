# Blend Lab

A small native C11 recipe service. It divides a target mass into two to eight weighted components, preserves the exact integer total, and awards leftover grams by largest remainder with stable input-order ties. Values are synthetic recipe quantities; no external services, storage, accounts or credentials are needed.

`GET /blend/1001/5000,3000,2000` returns allocations `[501,300,200]`. Weights are integer basis points summing to 10000; zero-weight components are supported. Mass is 1..1000000 grams. Query parameters do not affect the recipe. `/health` describes limits and `/recipe` gives an example.

The established native application contract is one raw HTTP request line on stdin and one JSON value on stdout, then process exit. Domain errors use `ok:false` and a stable error code; the platform supplies HTTP 200. This application does not claim HTTP 4xx semantics for that native contract. Diagnostic logs go to stderr.

For local use: `cmake -S . -B build && cmake --build build`, then `./build/blend_lab --serve`. This loopback listener uses port 8290 and exactly the same handler. Without `--serve`, the executable uses the platform's one-request protocol. No socket listener is compiled for WASI.
