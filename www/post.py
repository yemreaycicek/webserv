import sys, os

length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length)

print("Content-Type: text/plain")
print()
print("Method: " + os.environ.get("REQUEST_METHOD", "?"))
print("Gelen govde (" + str(length) + " bayt): " + body)