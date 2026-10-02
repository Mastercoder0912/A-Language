import sys

if len(sys.argv) >= 3:
    print(f"args:{sys.argv[1]}|{sys.argv[2]}")
else:
    print("args:missing")
