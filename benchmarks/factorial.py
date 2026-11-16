import time
import sys


def factorial(n):
    result = 1
    i = 2
    while i <= n:
        result = result * i
        i += 1
    return result

def main():
    if len(sys.argv) > 1:
        runs = int(sys.argv[1])
    else:
        runs = 1
    best = float("inf")
    for _ in range(runs):
        start = time.perf_counter()
        i = 0
        while i < 100000:
            factorial(15)
            i += 1
        end = time.perf_counter()
        if end - start < best:
            best = end - start
    print(f"Time taken: {best:.6f} seconds")

if __name__ == "__main__":
    main()
