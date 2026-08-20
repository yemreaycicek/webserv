#!/usr/bin/env python3
"""
HTTP-level test suite for the webserv project.

Talks to the server with raw TCP sockets only (Python standard library, no
third-party dependencies) so it exercises exactly what goes over the wire:
status lines, headers, chunked bodies, malformed input, and timing behaviour
of the non-blocking event loop.

Run it against a server started with tester/webserv_test.conf, e.g. via
tester/run.sh, or directly:

    ./bin/webserv tester/webserv_test.conf &
    python3 tester/webserv_tester.py
"""

import argparse
import glob
import os
import socket
import sys
import threading
import time

HOST = "127.0.0.1"
MAIN_PORT = 8080
LIMITED_PORT = 8081
TIMEOUT = 5.0

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UPLOAD_DIR = os.path.join(REPO_ROOT, "tester", "uploads", "files")


# --------------------------------------------------------------------------
# Minimal HTTP/1.x plumbing
# --------------------------------------------------------------------------

class SkipTest(Exception):
    pass


class HttpResponse(object):
    def __init__(self, raw):
        self.raw = raw
        self.status = None
        self.reason = ""
        self.headers = {}
        self.body = b""
        self._parse(raw)

    def _parse(self, raw):
        header_end = raw.find(b"\r\n\r\n")
        head = raw if header_end == -1 else raw[:header_end]
        self.body = b"" if header_end == -1 else raw[header_end + 4:]
        lines = head.split(b"\r\n")
        if lines and lines[0]:
            parts = lines[0].split(b" ", 2)
            if len(parts) >= 2:
                try:
                    self.status = int(parts[1])
                except ValueError:
                    self.status = None
                if len(parts) > 2:
                    self.reason = parts[2].decode("latin-1")
        for line in lines[1:]:
            if b":" in line:
                k, v = line.split(b":", 1)
                self.headers[k.strip().lower().decode("latin-1")] = v.strip().decode("latin-1")

    def text(self):
        return self.body.decode("utf-8", errors="replace")


def build_request(method, path, headers=None, body=b"", version="HTTP/1.1", host=None):
    if isinstance(body, str):
        body = body.encode("utf-8")
    hdrs = {}
    hdrs["Host"] = host if host is not None else HOST
    if body and "Content-Length" not in (headers or {}) and (not headers or "Transfer-Encoding" not in headers):
        hdrs["Content-Length"] = str(len(body))
    if headers:
        hdrs.update(headers)
    lines = ["%s %s %s" % (method, path, version)]
    for k, v in hdrs.items():
        lines.append("%s: %s" % (k, v))
    head = ("\r\n".join(lines) + "\r\n\r\n").encode("latin-1")
    return head + body


def raw_request(host, port, data, timeout=TIMEOUT):
    """Send raw bytes on a fresh connection and read until the peer closes
    the socket or the timeout elapses. Returns (bytes_received, timed_out)."""
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.settimeout(timeout)
    chunks = []
    timed_out = False
    try:
        sock.sendall(data)
        while True:
            try:
                chunk = sock.recv(65536)
            except socket.timeout:
                timed_out = True
                break
            if not chunk:
                break
            chunks.append(chunk)
    finally:
        sock.close()
    return b"".join(chunks), timed_out


def do_request(method, path, host=None, port=None, headers=None, body=b"", version="HTTP/1.1", timeout=TIMEOUT):
    host = HOST if host is None else host
    port = MAIN_PORT if port is None else port
    req = build_request(method, path, headers=headers, body=body, version=version, host=host)
    raw, timed_out = raw_request(host, port, req, timeout=timeout)
    return HttpResponse(raw), timed_out


def chunked_body(parts):
    out = b""
    for p in parts:
        if isinstance(p, str):
            p = p.encode("utf-8")
        out += ("%x\r\n" % len(p)).encode("ascii") + p + b"\r\n"
    out += b"0\r\n\r\n"
    return out


def cleanup_upload(name):
    path = os.path.join(UPLOAD_DIR, name)
    try:
        os.remove(path)
    except OSError:
        pass


# --------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------

def test_get_static_file():
    resp, timed_out = do_request("GET", "/index.html")
    assert not timed_out, "no response received"
    assert resp.status == 200, "expected 200, got %r" % resp.status
    assert "TESTER_INDEX_OK" in resp.text(), "unexpected body: %r" % resp.text()[:200]
    assert resp.headers.get("content-type", "").startswith("text/html"), \
        "expected text/html content-type, got %r" % resp.headers.get("content-type")


def test_get_missing_file_uses_custom_404():
    resp, timed_out = do_request("GET", "/does-not-exist.html")
    assert not timed_out, "no response received"
    assert resp.status == 404, "expected 404, got %r" % resp.status
    assert "TESTER_CUSTOM_404" in resp.text(), \
        "custom error_page from config was not used, got body: %r" % resp.text()[:200]


def test_autoindex_listing():
    resp, timed_out = do_request("GET", "/gallery/")
    assert not timed_out, "no response received"
    assert resp.status == 200, "expected 200 for autoindex, got %r" % resp.status
    assert "photo1.txt" in resp.text(), "autoindex listing missing known file: %r" % resp.text()[:300]


def test_directory_without_index_or_autoindex_is_forbidden():
    resp, timed_out = do_request("GET", "/noindex/")
    assert not timed_out, "no response received"
    assert resp.status == 403, "expected 403 for directory with no index/autoindex, got %r" % resp.status


def test_method_not_allowed_on_get_only_location():
    resp, timed_out = do_request("POST", "/getonly/file.txt", body="x")
    assert not timed_out, "no response received"
    assert resp.status == 405, "expected 405, got %r" % resp.status


def test_unknown_method_returns_501():
    raw = b"PATCH / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"
    data, timed_out = raw_request(HOST, MAIN_PORT, raw)
    assert not timed_out or data, \
        "server hung instead of responding to an unrecognised method (PATCH)"
    resp = HttpResponse(data)
    assert resp.status == 501, "expected 501 Not Implemented, got %r" % resp.status


def test_unsupported_http_version_returns_505():
    raw = b"GET / HTTP/9.9\r\nHost: 127.0.0.1\r\n\r\n"
    data, timed_out = raw_request(HOST, MAIN_PORT, raw)
    assert not timed_out or data, "server hung instead of responding to HTTP/9.9"
    resp = HttpResponse(data)
    assert resp.status == 505, "expected 505 HTTP Version Not Supported, got %r" % resp.status


def test_malformed_request_line_returns_400():
    raw = b"GET /\r\nHost: 127.0.0.1\r\n\r\n"  # missing HTTP version token
    data, timed_out = raw_request(HOST, MAIN_PORT, raw)
    assert not timed_out or data, (
        "server never responded to a malformed request line and the connection "
        "just hung open. The request parser correctly flags this as an error "
        "internally (Request::hasError()), but the caller in "
        "Server::handleCl() only checks isRequestComplete() and never checks "
        "hasError(), so an errored request is silently never answered."
    )
    resp = HttpResponse(data)
    assert resp.status == 400, "expected 400 Bad Request, got %r" % resp.status


def test_malformed_header_returns_400():
    raw = b"GET / HTTP/1.1\r\nHost 127.0.0.1\r\n\r\n"  # missing colon
    data, timed_out = raw_request(HOST, MAIN_PORT, raw)
    assert not timed_out or data, "server hung instead of responding to a malformed header line"
    resp = HttpResponse(data)
    assert resp.status == 400, "expected 400 Bad Request, got %r" % resp.status


def test_upload_get_delete_roundtrip():
    name = "tester_roundtrip.txt"
    upload_uri = "/upload/" + name
    fetch_uri = "/upload/files/" + name
    payload = "hello from webserv tester\n"
    try:
        resp, timed_out = do_request("POST", upload_uri, body=payload)
        assert not timed_out, "no response to POST upload"
        assert resp.status == 201, "expected 201 Created for upload, got %r" % resp.status

        resp, timed_out = do_request("GET", fetch_uri)
        assert not timed_out, "no response to GET of uploaded file"
        assert resp.status == 200, "expected 200 when reading back uploaded file, got %r" % resp.status
        assert resp.text() == payload, "uploaded content mismatch: %r" % resp.text()

        resp, timed_out = do_request("DELETE", fetch_uri)
        assert not timed_out, "no response to DELETE"
        assert resp.status == 200, "expected 200 for DELETE, got %r" % resp.status

        resp, timed_out = do_request("GET", fetch_uri)
        assert not timed_out, "no response to GET after delete"
        assert resp.status == 404, "expected 404 after delete, got %r" % resp.status
    finally:
        cleanup_upload(name)


def test_delete_nonexistent_file_returns_404():
    resp, timed_out = do_request("DELETE", "/upload/files/does-not-exist-at-all.txt")
    assert not timed_out, "no response received"
    assert resp.status == 404, "expected 404, got %r" % resp.status


def test_post_forbidden_when_upload_disabled():
    resp, timed_out = do_request("POST", "/noupload/newfile.txt", body="x")
    assert not timed_out, "no response received"
    assert resp.status == 403, (
        "expected 403 Forbidden when POSTing to a location that allows POST "
        "but has upload_enable off, got %r" % resp.status
    )


def test_chunked_upload_is_dechunked_correctly():
    name = "tester_chunked.txt"
    upload_uri = "/upload/" + name
    fetch_uri = "/upload/files/" + name
    parts = ["first-chunk-", "second-chunk-", "third"]
    expected = "".join(parts)
    try:
        req = build_request(
            "POST", upload_uri,
            headers={"Transfer-Encoding": "chunked"},
        )
        # build_request() would add Content-Length; strip the body-less
        # request line/headers and append a real chunked body instead.
        head = req.split(b"\r\n\r\n")[0] + b"\r\n\r\n"
        req = head + chunked_body(parts)

        data, timed_out = raw_request(HOST, MAIN_PORT, req)
        assert not timed_out, "no response to chunked POST"
        resp = HttpResponse(data)
        assert resp.status == 201, "expected 201 Created for chunked upload, got %r" % resp.status

        resp, timed_out = do_request("GET", fetch_uri)
        assert not timed_out, "no response reading back chunked upload"
        assert resp.status == 200, "expected 200, got %r" % resp.status
        assert resp.text() == expected, \
            "dechunked content mismatch: expected %r got %r" % (expected, resp.text())
    finally:
        cleanup_upload(name)


def test_body_over_limit_returns_413():
    body = "x" * 100  # server on LIMITED_PORT has client_max_body_size 20
    resp, timed_out = do_request("POST", "/", host=HOST, port=LIMITED_PORT, body=body)
    assert not timed_out, "no response received"
    assert resp.status == 413, "expected 413 Content Too Large, got %r" % resp.status


def test_body_within_limit_is_accepted():
    name = "tester_small_limit.txt"
    body = "0123456789"  # 10 bytes, under the 20 byte limit on LIMITED_PORT
    try:
        resp, timed_out = do_request("POST", "/" + name, host=HOST, port=LIMITED_PORT, body=body)
        assert not timed_out, "no response received"
        assert resp.status == 201, "expected 201 Created, got %r" % resp.status
    finally:
        cleanup_upload(name)


def test_second_listen_port_serves_independently():
    resp, timed_out = do_request("GET", "/index.html", port=LIMITED_PORT)
    assert not timed_out, "no response received"
    assert resp.status == 200, "second listen port did not answer a plain GET, got %r" % resp.status


def test_path_traversal_is_blocked():
    traversal = "/" + "../" * 20 + "etc/passwd"
    resp, timed_out = do_request("GET", traversal)
    assert not timed_out, "no response received"
    assert "root:" not in resp.text(), (
        "GET %s leaked %s: root path is built by concatenating the URI onto "
        "the location root without normalising '..' segments "
        "(Resolver::buildFsPath), which allows escaping the document root."
        % (traversal, "/etc/passwd")
    )
    assert resp.status != 200, \
        "expected traversal attempt to be rejected, got 200: %r" % resp.text()[:200]


def test_query_string_is_stripped_from_path(): # edge case
    resp, timed_out = do_request("GET", "/index.html?foo=bar")
    assert not timed_out, "no response received"
    assert resp.status == 200, (
        "expected the query string to be stripped before resolving the "
        "filesystem path, got %r (RequestLine keeps the raw request-target, "
        "including '?...', and Resolver appends it straight onto the root)"
        % resp.status
    )


def test_redirect_location_returns_301(): # edge case
    resp, timed_out = do_request("GET", "/old-site")
    assert not timed_out, "no response received"
    assert resp.status == 301, (
        "expected 301 for a 'return 301 /' location, got %r. The 'return' "
        "directive is parsed into LocationBlock::redirect but nothing in "
        "Executor/Resolver currently reads it." % resp.status
    )
    assert resp.headers.get("location") == "/", \
        "expected Location: / header, got %r" % resp.headers.get("location")


def test_slow_client_does_not_block_other_clients():
    """Trickle a request one byte at a time on one connection while a second,
    fully-formed request runs concurrently. The subject requires a single
    non-blocking poll()/select()/epoll() driving all client I/O, so the fast
    request must not be held up by the slow one."""
    req = b"GET /index.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n"

    def slow_client():
        s = socket.create_connection((HOST, MAIN_PORT), timeout=TIMEOUT + 5)
        try:
            for i in range(len(req)):
                s.sendall(req[i:i + 1])
                time.sleep(0.05)
            s.settimeout(TIMEOUT)
            try:
                while s.recv(4096):
                    pass
            except socket.timeout:
                pass
        finally:
            s.close()

    t = threading.Thread(target=slow_client)
    t.daemon = True
    t.start()
    time.sleep(0.3)  # let the slow client start trickling bytes

    start = time.time()
    resp, timed_out = do_request("GET", "/index.html")
    elapsed = time.time() - start
    t.join(TIMEOUT + 5)

    assert not timed_out, "fast request timed out while a slow client was connected"
    assert resp.status == 200, "expected 200, got %r" % resp.status
    assert elapsed < 1.5, (
        "a plain GET took %.2fs to answer while a slow client was still "
        "trickling its request - the server may be blocking on one "
        "connection's I/O instead of multiplexing with a single poll()"
        % elapsed
    )


def test_many_concurrent_clients():
    n = 40
    results = [None] * n
    errors = [None] * n

    def worker(i):
        try:
            resp, timed_out = do_request("GET", "/index.html", timeout=TIMEOUT)
            results[i] = (resp.status, timed_out)
        except Exception as e:
            errors[i] = e

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    start = time.time()
    for th in threads:
        th.start()
    for th in threads:
        th.join(TIMEOUT + 5)
    elapsed = time.time() - start

    failed = [i for i in range(n) if errors[i] is not None]
    assert not failed, "connection errors under concurrent load: %r" % ([errors[i] for i in failed][:3])
    bad = [r for r in results if r != (200, False)]
    assert not bad, "%d/%d concurrent requests failed or timed out" % (len(bad), n)
    assert elapsed < TIMEOUT + 5, "concurrent batch of %d requests took %.2fs" % (n, elapsed)


# --------------------------------------------------------------------------
# Runner
# --------------------------------------------------------------------------

TESTS = [
    ("static file GET", test_get_static_file, "core"),
    ("custom 404 error page", test_get_missing_file_uses_custom_404, "core"),
    ("autoindex directory listing", test_autoindex_listing, "core"),
    ("directory forbidden w/o index or autoindex", test_directory_without_index_or_autoindex_is_forbidden, "core"),
    ("405 on disallowed method", test_method_not_allowed_on_get_only_location, "core"),
    ("501 on unknown method", test_unknown_method_returns_501, "core"),
    ("505 on unsupported HTTP version", test_unsupported_http_version_returns_505, "core"),
    ("400 on malformed request line", test_malformed_request_line_returns_400, "core"),
    ("400 on malformed header", test_malformed_header_returns_400, "core"),
    ("upload/GET/DELETE roundtrip", test_upload_get_delete_roundtrip, "core"),
    ("404 deleting nonexistent file", test_delete_nonexistent_file_returns_404, "core"),
    ("403 POST when upload disabled", test_post_forbidden_when_upload_disabled, "core"),
    ("chunked transfer-encoding upload", test_chunked_upload_is_dechunked_correctly, "core"),
    ("413 body over client_max_body_size", test_body_over_limit_returns_413, "core"),
    ("201 body within client_max_body_size", test_body_within_limit_is_accepted, "core"),
    ("second listen port answers", test_second_listen_port_serves_independently, "core"),
    ("path traversal blocked", test_path_traversal_is_blocked, "security"),
    ("query string stripped from path", test_query_string_is_stripped_from_path, "edge"),
    ("'return' directive issues 301", test_redirect_location_returns_301, "edge"),
    ("slow client doesn't block others", test_slow_client_does_not_block_other_clients, "core"),
    ("many concurrent clients", test_many_concurrent_clients, "core"),
]


def wait_for_port(host, port, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            s = socket.create_connection((host, port), timeout=0.5)
            s.close()
            return True
        except (socket.error, OSError):
            time.sleep(0.1)
    return False


def main():
    global HOST, MAIN_PORT, LIMITED_PORT

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--main-port", type=int, default=MAIN_PORT)
    parser.add_argument("--limited-port", type=int, default=LIMITED_PORT)
    parser.add_argument("--wait", type=float, default=0,
                         help="seconds to wait for the server ports to accept connections before testing")
    parser.add_argument("--tag", action="append",
                         help="only run tests with this tag (core, security, edge); repeatable")
    args = parser.parse_args()

    HOST, MAIN_PORT, LIMITED_PORT = args.host, args.main_port, args.limited_port

    for name in glob.glob(os.path.join(UPLOAD_DIR, "tester_*.txt")):
        try:
            os.remove(name)
        except OSError:
            pass

    if args.wait:
        for port in (MAIN_PORT, LIMITED_PORT):
            if not wait_for_port(HOST, port, args.wait):
                print("error: nothing listening on %s:%d after %.1fs" % (HOST, port, args.wait))
                return 2

    selected = TESTS
    if args.tag:
        selected = [t for t in TESTS if t[2] in args.tag]

    results = []
    for name, func, tag in selected:
        try:
            func()
            results.append((name, tag, "PASS", ""))
        except SkipTest as e:
            results.append((name, tag, "SKIP", str(e)))
        except AssertionError as e:
            results.append((name, tag, "FAIL", str(e)))
        except Exception as e:
            results.append((name, tag, "FAIL", "unexpected error: %r" % (e,)))

    width = max(len(r[0]) for r in results) + 1
    print("\n%s" % ("-" * (width + 30)))
    for name, tag, status, msg in results:
        print("[%-4s] (%-8s) %-*s %s" % (status, tag, width, name, msg if status == "FAIL" else ""))
    print("%s" % ("-" * (width + 30)))

    passed = sum(1 for r in results if r[2] == "PASS")
    failed = [r for r in results if r[2] == "FAIL"]
    skipped = sum(1 for r in results if r[2] == "SKIP")
    print("%d passed, %d failed, %d skipped (%d total)\n" % (passed, len(failed), skipped, len(results)))

    core_failures = [r for r in failed if r[1] == "core"]
    return 1 if core_failures else 0


if __name__ == "__main__":
    sys.exit(main())
