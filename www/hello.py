import sys, os
body = sys.stdin.read()
print("Content-Type: text/plain")
print()
print("Merhaba Dünya")
print("method:", os.environ.get("REQUEST_METHOD"))
print("query:", os.environ.get("QUERY_STRING"))
print("body:", repr(body))
