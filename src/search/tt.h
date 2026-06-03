#pragma once
#include <cstdint>

enum TTFlag : uint8_t { EXACT, LOWERBOUND, UPPERBOUND };

struct TTEntry {
    uint64_t key;
    int score;
    int8_t depth;
    TTFlag flag;
};

static const int TT_SIZE = 1 << 20; // 1M entries ~16MB
static TTEntry tt[TT_SIZE];

static TTEntry* tt_probe(uint64_t key) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    return (e.key == key) ? &e : nullptr;
}

static void tt_store(uint64_t key, int depth, int score, TTFlag flag) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    e = {key, score, (int8_t)depth, flag};
}