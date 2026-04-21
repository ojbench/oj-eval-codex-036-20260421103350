#include <vector>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <string>

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
        for (std::size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (c == '1') set(i, true);
            else set(i, false);
        }
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
        // append at current highest position (index n), then increase length
        ensure_capacity(n + 1);
        std::size_t bi = n / B, off = n % B;
        if (val) a[bi] |= (1ULL << off); // default is 0, so only set if true
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
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) {
            a[i] |= o.a[i];
        }
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
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) {
            a[i] &= o.a[i];
        }
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
        for (std::size_t i = 0; i < full && i < a.size() && i < o.a.size(); ++i) {
            a[i] ^= o.a[i];
        }
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
        if (n == 0) { resize_zero(0); n = 0; return *this; }
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
        // mask off bits beyond n in last block
        std::size_t rem = n % B;
        if (rem) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            a.back() &= mask;
        }
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
            if (shift && i + 1 < old_blocks && j < new_blocks) {
                // bring in high bits from the next block
                r[j] |= (a[i + 1] << (B - shift));
            }
        }
        a.swap(r);
        n = new_n;
        // mask off bits beyond n in last block
        std::size_t rem = n % B;
        if (rem) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            a.back() &= mask;
        }
        return *this;
    }

    dynamic_bitset &set() {
        std::size_t blocks = a.size();
        for (std::size_t i = 0; i < blocks; ++i) a[i] = ~0ULL;
        std::size_t rem = n % B;
        if (rem) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            a.back() &= mask;
        }
        return *this;
    }

    dynamic_bitset &flip() {
        std::size_t blocks = a.size();
        for (std::size_t i = 0; i < blocks; ++i) a[i] = ~a[i];
        std::size_t rem = n % B;
        if (rem) {
            std::uint64_t mask = (rem == 64 ? ~0ULL : ((1ULL << rem) - 1ULL));
            a.back() &= mask;
        }
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
};

// Helper: print bitset with lowest bit first
static std::string to_string_low_first(const dynamic_bitset &db) {
    std::string s; s.reserve(db.size());
    for (std::size_t i = 0; i < db.size(); ++i) s.push_back(db[i] ? '1' : '0');
    return s;
}

// Simple registry mapping string ids to bitsets without extra headers
static int find_id(const std::vector<std::string> &names, const std::string &id) {
    for (std::size_t i = 0; i < names.size(); ++i) if (names[i] == id) return (int)i;
    return -1;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Flexible command interpreter
    // Supported forms:
    // new <id> <n>
    // new <id> <bits_string>
    // get <id> <pos>
    // set <id> <pos> <0|1>
    // push|push_back <id> <0|1>
    // none|all|size <id>
    // or|and|xor <ida> <idb>
    // lshift|rshift <id> <n>
    // setall|flip|reset <id>
    // print <id>

    std::vector<std::string> names;
    std::vector<dynamic_bitset> sets;

    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "new") {
            std::string id, val; std::cin >> id >> val;
            bool is_num = !val.empty() && std::all_of(val.begin(), val.end(), [](char c){return c>='0' && c<='9';});
            int idx = find_id(names, id);
            if (is_num) {
                std::size_t n = 0; for (char c: val) { n = n*10 + (c-'0'); }
                if (idx == -1) { names.push_back(id); sets.emplace_back(n); }
                else sets[idx] = dynamic_bitset(n);
            } else {
                if (idx == -1) { names.push_back(id); sets.emplace_back(val); }
                else sets[idx] = dynamic_bitset(val);
            }
        } else if (cmd == "get") {
            std::string id; std::size_t pos; std::cin >> id >> pos;
            int idx = find_id(names, id);
            bool v = (idx >= 0) ? sets[idx][pos] : false;
            std::cout << (v ? 1 : 0) << '\n';
        } else if (cmd == "set") {
            std::string id; std::size_t pos; int v; std::cin >> id >> pos >> v;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx].set(pos, v != 0);
        } else if (cmd == "push" || cmd == "push_back") {
            std::string id; int v; std::cin >> id >> v;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx].push_back(v != 0);
        } else if (cmd == "none") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            std::cout << (sets[idx].none() ? 1 : 0) << '\n';
        } else if (cmd == "all") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            std::cout << (sets[idx].all() ? 1 : 0) << '\n';
        } else if (cmd == "size") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            std::cout << sets[idx].size() << '\n';
        } else if (cmd == "or") {
            std::string A,B; std::cin >> A >> B;
            int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
            int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
            sets[ia] |= sets[ib];
        } else if (cmd == "and") {
            std::string A,B; std::cin >> A >> B;
            int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
            int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
            sets[ia] &= sets[ib];
        } else if (cmd == "xor") {
            std::string A,B; std::cin >> A >> B;
            int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
            int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
            sets[ia] ^= sets[ib];
        } else if (cmd == "lshift") {
            std::string id; std::size_t k; std::cin >> id >> k;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx] <<= k;
        } else if (cmd == "rshift") {
            std::string id; std::size_t k; std::cin >> id >> k;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx] >>= k;
        } else if (cmd == "setall") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx].set();
        } else if (cmd == "flip") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx].flip();
        } else if (cmd == "reset") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            sets[idx].reset();
        } else if (cmd == "print") {
            std::string id; std::cin >> id;
            int idx = find_id(names, id);
            if (idx == -1) { names.push_back(id); sets.emplace_back(0); idx = (int)sets.size()-1; }
            std::cout << to_string_low_first(sets[idx]) << '\n';
        } else {
            // try simple symbolic forms
            // e.g., A|=B, A&=B, A^=B
            if (cmd.find("|=") != std::string::npos) {
                auto p = cmd.find("|=");
                std::string A = cmd.substr(0,p); std::string B = cmd.substr(p+2);
                int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
                int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
                sets[ia] |= sets[ib];
            } else if (cmd.find("&=") != std::string::npos) {
                auto p = cmd.find("&=");
                std::string A = cmd.substr(0,p); std::string B = cmd.substr(p+2);
                int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
                int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
                sets[ia] &= sets[ib];
            } else if (cmd.find("^=") != std::string::npos) {
                auto p = cmd.find("^=");
                std::string A = cmd.substr(0,p); std::string B = cmd.substr(p+2);
                int ia = find_id(names, A); if (ia == -1) { names.push_back(A); sets.emplace_back(0); ia = (int)sets.size()-1; }
                int ib = find_id(names, B); if (ib == -1) { names.push_back(B); sets.emplace_back(0); ib = (int)sets.size()-1; }
                sets[ia] ^= sets[ib];
            } else {
                // unknown command: ignore line
                std::string rest; std::getline(std::cin, rest);
            }
        }
    }
    return 0;
}
