#!/usr/bin/env bash
# Builds webserv, starts it with tester/webserv_test.conf, runs the Python
# test suite against it, and tears the server down afterwards.
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
CONF="$SCRIPT_DIR/webserv_test.conf"
BIN="bin/webserv"
LOG="$SCRIPT_DIR/server.log"

cd "$ROOT_DIR" || exit 1

echo "==> building webserv"
if ! make -C "$ROOT_DIR" all >/tmp/webserv_tester_build.log 2>&1; then
    echo "build failed, see /tmp/webserv_tester_build.log"
    tail -n 40 /tmp/webserv_tester_build.log
    exit 1
fi

echo "==> starting $BIN $CONF"
"$ROOT_DIR/$BIN" "$CONF" >"$LOG" 2>&1 &
SERVER_PID=$!

cleanup() {
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        for _ in $(seq 1 20); do
            kill -0 "$SERVER_PID" 2>/dev/null || break
            sleep 0.1
        done
        kill -9 "$SERVER_PID" 2>/dev/null
    fi
}
trap cleanup EXIT

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "server exited immediately, see $LOG"
    cat "$LOG"
    exit 1
fi

echo "==> running tests"
python3 "$SCRIPT_DIR/webserv_tester.py" --wait 5 "$@"
STATUS=$?

if [ "$STATUS" -ne 0 ]; then
    echo "==> server log (last 40 lines):"
    tail -n 40 "$LOG"
fi

exit "$STATUS"
