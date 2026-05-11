# Cache Simulator

A C++ cache simulator that models a two-level memory hierarchy with configurable L1 and L2 caches.  
The simulator receives a memory access trace and calculates L1 miss rate, L2 miss rate and average access time.

## Overview

This project was developed as an academic systems programming assignment.  
It simulates cache behavior for read and write operations using configurable cache parameters such as: block size, cache size, associativity, access cycles, memory access time and write allocation policy.

## Build

```bash
make
```
or directly:
```bash
g++ -Wall -Wextra -std=c++17 -O2 -o cacheSim cacheSim.cpp
```

## Example

```bash
./cacheSim examples/example1_trace \
  --mem-cyc 100 \
  --bsize 5 \
  --l1-size 12 \
  --l2-size 16 \
  --l1-assoc 2 \
  --l2-assoc 4 \
  --l1-cyc 1 \
  --l2-cyc 10 \
  --wr-alloc 1
```

Expected output format:
```bash
L1miss=0.xxx L2miss=0.xxx AccTimeAvg=xx.xxx
```