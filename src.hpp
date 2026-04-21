// Implementation header for sjtu::dynamic_bitset
#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>

namespace sjtu {

struct dynamic_bitset {
    static constexpr std::size_t B = 64;
    std::vector<std::uint64_t> a;
    std::size_t n = 0;

    dynamic_bitset() = default;
    ~dynamic_bitset() = default;
    dynamic_bitset(const dynamic_bitset &) = default;
    dynamic_bitset &operator=(const dynamic_bitset &) = default;

    dynamic_bitset(std::size_t n_) { resize_zero(n_); }
    dynamic_bitset(const std::string &str) {
        resize_zero(str.size());
        for (std::size_t i = 0; i < str.size(); ++i) set(i, str[i] == '1');
    }

    bool operator[](std::size_t idx) const {
        if (idx >= n) return false;
        std::size_t bi = idx / B, off = idx % B;
        return (a[bi] >> off) & 1ULL;
    }

    dynamic_bitset &set(std::size_t idx, bool val = true) {
        if (idx >= n) return *this;
        std::size_t bi = idx / B, off = idx % B;
        std::uint64_t mask = (1ULL << off);
        if (val) a[bi] |= mask; else a[bi] &= ~mask;
        return *this;
    }

    dynamic_bitset &push_back(bool val) {
        ensure_capacity(n + 1);
        if (val) a[n / B] |= (1ULL << (n % B));
        ++n;
        return *this;
    }

    bool none() const {
        if (n == 0) return true;
        std::size_t full = n / B;
        for (std::size_t i = 0; i < full; ++i) if (a[i] != 0ULL) return false;
        std::size_t rem = n % B;
        if (rem == 0) return true;
        std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
        return (a[full] & mask) == 0ULL;
    }

    bool all() const {
        if (n == 0) return true;
        std::size_t full = n / B;
        for (std::size_t i = 0; i < full; ++i) if (a[i] != ~0ULL) return false;
        std::size_t rem = n % B;
        if (rem == 0) return true;
        std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
        return (a[full] & mask) == mask;
    }

    std::size_t size() const { return n; }

    dynamic_bitset &operator|=(const dynamic_bitset &o) {
        std::size_t common = std::min(n, o.n);
        std::size_t full = common / B;
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) a[i] |= o.a[i];
        std::size_t rem = common % B;
        if (rem && full < a.size() && full < o.a.size()) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            std::uint64_t keep = a[full] & ~mask;
            std::uint64_t merged = (a[full] & mask) | (o.a[full] & mask);
            a[full] = keep | merged;
        }
        return *this;
    }

    dynamic_bitset &operator&=(const dynamic_bitset &o) {
        std::size_t common = std::min(n, o.n);
        std::size_t full = common / B;
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) a[i] &= o.a[i];
        std::size_t rem = common % B;
        if (rem && full < a.size()) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            std::uint64_t keep = a[full] & ~mask;
            std::uint64_t merged = (a[full] & mask) & (o.a.size() > full ? (o.a[full] & mask) : 0ULL);
            a[full] = keep | merged;
        }
        return *this;
    }

    dynamic_bitset &operator^=(const dynamic_bitset &o) {
        std::size_t common = std::min(n, o.n);
        std::size_t full = common / B;
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) a[i] ^= o.a[i];
        std::size_t rem = common % B;
        if (rem && full < a.size() && full < o.a.size()) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            std::uint64_t keep = a[full] & ~mask;
            std::uint64_t merged = (a[full] & mask) ^ (o.a[full] & mask);
            a[full] = keep | merged;
        }
        return *this;
    }

    dynamic_bitset &operator<<=(std::size_t m) {
        if (m == 0) return *this;
        if (n == 0) { resize_zero(0); return *this; }
        std::size_t new_n = n + m;
        std::size_t add_blocks = m / B;
        std::size_t shift = m % B;
        std::size_t old_blocks = a.size();
        std::size_t new_blocks = (new_n + B - 1) / B;
        std::vector<std::uint64_t> r(new_blocks, 0ULL);
        for (std::size_t i = 0; i < old_blocks; ++i) {
            std::size_t j = i + add_blocks;
            if (j < new_blocks) r[j] |= (a[i] << shift);
            if (shift && j + 1 < new_blocks) r[j + 1] |= (a[i] >> (B - shift));
        }
        a.swap(r);
        n = new_n;
        trim_last();
        return *this;
    }

    dynamic_bitset &operator>>=(std::size_t m) {
        if (m == 0) return *this;
        if (m >= n) { resize_zero(0); return *this; }
        std::size_t new_n = n - m;
        std::size_t drop_blocks = m / B;
        std::size_t shift = m % B;
        std::size_t old_blocks = a.size();
        std::size_t new_blocks = (new_n + B - 1) / B;
        std::vector<std::uint64_t> r(new_blocks, 0ULL);
        for (std::size_t i = drop_blocks; i < old_blocks; ++i) {
            std::size_t j = i - drop_blocks;
            if (j < new_blocks) r[j] |= (a[i] >> shift);
            if (shift && i + 1 < old_blocks && j < new_blocks) r[j] |= (a[i + 1] << (B - shift));
        }
        a.swap(r);
        n = new_n;
        trim_last();
        return *this;
    }

    dynamic_bitset &set() {
        for (auto &x : a) x = ~0ULL;
        trim_last();
        return *this;
    }

    dynamic_bitset &flip() {
        for (auto &x : a) x = ~x;
        trim_last();
        return *this;
    }

    dynamic_bitset &reset() {
        std::fill(a.begin(), a.end(), 0ULL);
        return *this;
    }

private:
    void ensure_capacity(std::size_t want_bits) {
        std::size_t need_blocks = (want_bits + B - 1) / B;
        if (a.size() < need_blocks) a.resize(need_blocks, 0ULL);
    }
    void resize_zero(std::size_t new_n) {
        n = new_n;
        a.assign((n + B - 1) / B, 0ULL);
    }
    void trim_last() {
        if (n == 0 || a.empty()) return;
        std::size_t rem = n % B;
        if (rem) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            a.back() &= mask;
        }
    }
};

} // namespace sjtu
