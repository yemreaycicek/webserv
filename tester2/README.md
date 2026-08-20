# webserv tester

A small HTTP-level test suite for this project's `webserv` binary. It speaks
raw TCP/HTTP directly (Python standard library only, no `requests` or other
dependencies) so it can exercise things a normal HTTP client library would
hide from you: exact status lines, chunked bodies, malformed requests, and
the timing behaviour of the non-blocking event loop.

## Layout

- `webserv_test.conf` — dedicated config for the tester. Two `server` blocks:
  - `127.0.0.1:8080` — the main block, mirrors `conf/default.conf` plus a few
    extra locations needed for coverage (`/noindex`, `/getonly`, `/noupload`).
  - `127.0.0.1:8081` — a second block with `client_max_body_size 20` to test
    the 413 path quickly, and to prove multiple listen ports work
    independently.
- `www/`, `uploads/`, `error_pages/` — fixture files served by the config
  above.
- `webserv_tester.py` — the test suite itself.
- `run.sh` — builds the project, starts the server with `webserv_test.conf`,
  runs the suite, and shuts the server down afterwards.

## Running

```sh
tester/run.sh
```

This builds `webserv`, launches it, waits for both ports to come up, runs the
tests, prints a summary, and stops the server. Exit code is non-zero if any
test tagged `core` failed.

To run against a server you already started yourself (e.g. from another
terminal, for debugging under a debugger):

```sh
./bin/webserv tester/webserv_test.conf &
python3 tester/webserv_tester.py
```

Useful flags on `webserv_tester.py`:

- `--host`, `--main-port`, `--limited-port` — point it at a different address.
- `--tag core` / `--tag security` / `--tag edge` (repeatable) — run only a
  subset. `core` is what the subject requires; `security` and `edge` cover
  extra checks (see below).

## What it checks

Most tests are tagged `core` and map directly to mandatory requirements:
static file serving, directory listing / autoindex, default & custom error
pages, `GET`/`POST`/`DELETE`, file upload, `client_max_body_size` (413),
chunked request bodies, multiple listen ports, and accurate status codes for
malformed input (400), unknown methods (501) and unsupported HTTP versions
(505).

Two tests specifically target the "single non-blocking poll() for all I/O"
requirement:

- **slow client doesn't block others** — trickles a request one byte at a
  time on one connection while a normal request runs on another connection
  at the same time, and asserts the fast one isn't held up.
- **many concurrent clients** — fires ~40 requests at once and checks they
  all come back correctly.

A couple of tests are tagged instead of `core`:

- `security`: **path traversal** — `GET /../../../etc/passwd`-style requests
  should not escape the configured document root.
- `edge`: **query string stripping** and the `return` (redirect) directive —
  both correspond to config/parsing features that may or may not be wired up
  yet depending on where the project stands; they're kept separate from
  `core` so a red result there doesn't block the rest of the suite.

If a test fails, its assertion message explains what was expected and, where
relevant, points at the file/function that looks responsible — that's a
starting point for investigation, not a verdict; always read the referenced
code before trusting the message.

## Cleaning up

Uploaded fixtures created during a run (`tester_*.txt` in
`uploads/files/`) are removed by each test in a `finally` block, and any
leftovers from a previous interrupted run are swept at the start of the next
one.
