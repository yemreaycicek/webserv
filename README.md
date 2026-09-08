*This project has been created as part of the 42 curriculum by akosaca, yaycicek.*

# webserv

A non-blocking HTTP/1.1 server written from scratch in C++98. It serves static
websites, handles file uploads, runs CGI scripts, and is driven by an
NGINX-inspired configuration file — all on a single event loop.

## Description

The goal of **webserv** is to understand HTTP by implementing a working web
server that a real browser can talk to. The server reads a configuration file,
opens one or more listening sockets, and multiplexes every client connection
through a **single `poll()` loop** — no operation on a socket, pipe, or CGI
descriptor is ever performed without first being reported ready by `poll()`.

Requests are parsed into an HTTP request object (request line, headers, body),
routed to the matching `server`/`location` block, and answered with an accurate
status code and response. The server never blocks and never crashes: slow
clients, disconnections, oversized bodies, and hanging CGI processes are all
handled without stalling the event loop.

The codebase is split into focused layers:

| Layer     | Responsibility                                                        |
| --------- | --------------------------------------------------------------------- |
| `config/` | Lexing, parsing and routing of the configuration file (`Lexer`, `Parser`, `Router`) |
| `http/`   | HTTP request model: request line, headers, body, status codes          |
| `net/`    | Low-level sockets and addresses (`Socket`, `ListenSocket`, `Address`)  |
| `exec/`   | The runtime: event loop, connections, response building and CGI (`Poller`, `Server`, `Connection`, `Resolver`, `ResponseBuilder`, `Cgi`) |
| `utils/`  | Argument handling, I/O and string helpers                              |

## Features

- **HTTP methods:** `GET`, `POST`, `DELETE`, `HEAD`
- **Static file serving** with a configurable root and default index file
- **Directory listing** (autoindex) that can be enabled per location
- **File uploads** from clients to a configurable storage directory
- **CGI execution** by file extension (e.g. Python via `python3`), using
  `fork` + `pipe` + `execve`, integrated non-blockingly into the `poll()` loop.
  If the CGI returns no `Content-Length`, `EOF` marks the end of the body.
- **HTTP redirections** (`return 301 /path;`)
- **Custom error pages** per status code, with built-in defaults when none are set
- **Request body size limits** (`client_max_body_size`), enforced both early
  (DoS protection) and after routing (per-location correctness)
- **Multiple listening `host:port` pairs** to serve different content
- **Clean shutdown** on `SIGINT`/`SIGTERM` and graceful handling of `SIGPIPE`

## Instructions

### Requirements

- A C++ compiler supporting **C++98** (`c++`)
- `make`
- `python3` (only if you want to run the included Python CGI examples)

### Build

```bash
make            # builds the binary at ./bin/webserv
make clean      # removes build/ (object files)
make fclean     # removes build/ and bin/
make re         # fclean + build
make debug      # rebuild with -DDEBUG and extra config debugging
```

The project compiles with `-Wall -Wextra -Werror -std=c++98`.

### Run

```bash
./bin/webserv [configuration_file]
```

- With no argument, the server falls back to the default config `conf/default.conf`.
- The configuration file **must** end with the `.conf` extension.

Examples:

```bash
./bin/webserv                     # uses conf/default.conf
./bin/webserv conf/other.conf     # multiple ports, multiple error pages
```

Then open `http://127.0.0.1:8080/` in your browser, or test with `curl`:

```bash
curl -v http://127.0.0.1:8080/
curl -X POST --data-binary @file.txt http://127.0.0.1:8080/upload
curl -X DELETE http://127.0.0.1:8080/upload/file.txt
```

## Configuration file

The configuration syntax is a subset inspired by NGINX's `server` block. A
`server` block defines one virtual server; each `location` block defines rules
for a URL prefix.

```nginx
server {
    listen 127.0.0.1:8080;              # one or more host:port pairs
    client_max_body_size 10m;           # max request body (bytes; k/m suffix allowed)
    error_page 404 /www/error_pages/404.html;

    location / {
        allow_methods GET;              # accepted HTTP methods for this route
        root www/html;                  # filesystem root for the route
        index index.html;              # default file when a directory is requested
        autoindex on;                   # directory listing on/off
    }

    location /old {
        return 301 /about.html;         # HTTP redirection
    }

    location /upload {
        allow_methods GET POST DELETE;
        root www/upload;
        upload_enable on;               # allow client uploads
        upload_store www/upload/files;  # where uploaded files are stored
        autoindex on;
    }

    location /cgi {
        allow_methods GET POST;
        root www;
        cgi_extension .py;              # extension that triggers CGI
        cgi_pass /usr/bin/python3;      # interpreter used to run the script
    }
}
```

### Directives

| Directive              | Scope             | Description                                              |
| ---------------------- | ----------------- | -------------------------------------------------------- |
| `listen`               | server            | `host:port` to listen on (can appear multiple times)     |
| `client_max_body_size` | server / location | Maximum request body size; plain bytes or `k` / `m` suffix |
| `error_page`           | server            | Custom page for a given status code                      |
| `allow_methods`        | location          | Space-separated list of accepted methods                 |
| `root`                 | location          | Filesystem directory the route maps to                   |
| `index`                | location          | Default file served for a directory request              |
| `autoindex`            | location          | `on` / `off` — enable directory listing                  |
| `return`               | location          | HTTP redirect, e.g. `return 301 /path;`                  |
| `upload_enable`        | location          | `on` / `off` — allow file uploads                        |
| `upload_store`         | location          | Directory where uploaded files are written               |
| `cgi_extension`        | location          | File extension handled as CGI                            |
| `cgi_pass`             | location          | Path to the CGI interpreter/binary                       |

Ready-to-use example configs live in `conf/` (`default.conf`, `other.conf`,
`tester.conf`), and matching web content lives in `www/`.

## Project structure

```
.
├── Makefile
├── conf/            # example configuration files
├── inc/             # headers, mirroring the src/ layout
│   ├── config/
│   ├── http/
│   ├── net/
│   ├── exec/
│   └── utils/
├── src/
│   ├── main.cpp
│   ├── config/      # Lexer, Parser, Router
│   ├── http/        # Request, RequestLine, Header, Body, Status
│   ├── net/         # Socket, ListenSocket, Address
│   ├── exec/        # Server, Poller, Connection, Resolver, ResponseBuilder, Cgi
│   └── utils/       # arg, io, str
├── www/             # static site, images, error pages, upload dirs, CGI scripts
```

## Testing

The server was validated against browsers, `curl`, and the following tools:

- **Official 42 tester** (`tester`) and the **YoupiBanane** test tree
  availability with 0 failed transactions
- **`valgrind --leak-check=full --show-leak-kinds=all --track-fds=yes`** to check
  for memory and file-descriptor leaks

Useful diagnostics during development:

```bash
ss -tan                 # inspect listening sockets
```

## Resources

Classic references used while building this project:

- **[MDN Web Docs](https://developer.mozilla.org/en-US/)** — HTTP overview, methods, status codes and headers
- **[NGINX Beginner's Guide](https://nginx.org/en/docs/beginners_guide.html)** —
  first look at how NGINX is configured and served
- **[NGINX Web Server admin guide](https://docs.nginx.com/nginx/admin-guide/web-server/web-server/)** —
  behavioural reference for the `server`/`location` model and directives
- **[NGINX directive index](https://nginx.org/en/docs/dirindex.html)** —
  alphabetical index of NGINX directives, used to look up individual directives
- **[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)** —
  sockets primer (socket/bind/listen/accept, non-blocking I/O)
- **[Common Gateway Interface (Wikipedia)](https://en.wikipedia.org/wiki/Common_Gateway_Interface)** —
  overview of CGI and the environment variables involved
- **`man` pages** — `poll`, `socket`, `bind`, `listen`, `accept`, `recv`,
  `send`, `fork`, `execve`, `pipe`, `dup2`, `waitpid`, `fcntl`

### Use of AI

AI assistance (a large language model) was used as a study and review aid,
never as a source of code copied without understanding:

- **Learning the underlying concepts** — the `poll()` readiness model,
  fork/pipe/execve mechanics for CGI, and HTTP message framing — by tracing
  concrete examples end to end.
- **Reviewing code against the subject's constraints**, e.g. avoiding `errno`
  after `read`/`write`, restricting `fcntl` to `F_SETFL`/`O_NONBLOCK`,
  and ensuring a single `poll()` drives all socket/pipe I/O.
- **Debugging specific issues**, such as a blocking `waitpid` (a DoS vector,
  fixed with `WNOHANG` + `SIGKILL`) and clean shutdown handling.
- **Drafting documentation**, including a first version of this README, which
  was then reviewed and adjusted by hand.

All AI-suggested changes were read, tested, and validated before being kept.
