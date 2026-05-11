#ifndef CACHE_SIM_HPP
#define CACHE_SIM_HPP

#include <string>
#include <vector>

struct Address {
    unsigned int initialAddress;
    unsigned int tag;
    unsigned int set;
    unsigned int offset;

    Address(unsigned int address, int numberOfSets, int blockSize);
};

class CacheLine {
public:
    bool isValid;
    bool isDirty;
    unsigned int tag;
    int lastUsed;

    CacheLine();
    CacheLine(bool valid, bool dirty, unsigned int newTag);
};

class CacheSet {
public:
    std::vector<CacheLine> lines;

    explicit CacheSet(int associativity);

    void updateLRU(unsigned int recentlyUsedIndex);
    int getLeastRecentlyUsedLine() const;
};

class Cache {
private:
    int blockSize;
    int cacheSize;
    int associativity;
    int accessCycles;
    std::vector<CacheSet> sets;

public:
    Cache* nextCache;
    Cache* previousCache;
    int numberOfSets;

    Cache(int blockSizeExp, int cacheSizeExp, int associativityExp, int accessCycles);

    int find(unsigned int address, bool writeAllocate, bool isWrite);
    int getLineToFill(int setIndex);
    void insert(unsigned int address, bool writeAllocate, bool isWrite);
};

#endif