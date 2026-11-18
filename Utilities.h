// Utilities.h - Unit IV: generic PriorityDispatchQueue<T>, functors, I/O manipulators
#pragma once
#include <algorithm>
#include <functional>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <vector>
#include "Inventory.h"

namespace ReliefOps {
namespace Util {

// Generic priority queue. Compare(a, b) == true means "a must be served BEFORE b".
template <typename T, typename Compare = std::less<T> >
class PriorityDispatchQueue {
    struct Lower {  // adapts "served before" into the max-heap ordering used by <algorithm>
        Compare cmp;
        bool operator()(const T& a, const T& b) const { return cmp(b, a); }
    };
public:
    explicit PriorityDispatchQueue(Compare cmp = Compare()) : cmp_(cmp) {}

    void push(const T& v) {
        heap_.push_back(v);
        std::push_heap(heap_.begin(), heap_.end(), Lower{cmp_});
    }
    T pop() {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        std::pop_heap(heap_.begin(), heap_.end(), Lower{cmp_});
        T v = heap_.back();
        heap_.pop_back();
        return v;
    }
    const T& top() const {
        if (heap_.empty()) throw std::out_of_range("PriorityDispatchQueue is empty");
        return heap_.front();
    }
    bool empty() const { return heap_.empty(); }
    std::size_t size() const { return heap_.size(); }
private:
    std::vector<T> heap_;
    Compare cmp_;
};

// Functor: most urgent medical need first (priority 1 beats 5); heavier load breaks ties.
struct MoreUrgentFirst {
    bool operator()(const Inventory::ReliefCargo& a, const Inventory::ReliefCargo& b) const {
        if (a.priority() != b.priority()) return a.priority() < b.priority();
        return a.totalWeightKg() > b.totalWeightKg();
    }
};

// ---- user-defined manipulators
struct ColWidth { int w; };
inline ColWidth col(int w) { ColWidth c = {w}; return c; }  // parameterised manipulator
inline std::ostream& operator<<(std::ostream& os, ColWidth c) { return os << std::left << std::setw(c.w); }

inline std::ostream& tableRule(std::ostream& os) { return os << std::string(66, '-') << '\n'; }
inline std::ostream& cargoHeader(std::ostream& os) {
    os << tableRule << col(18) << "ITEM" << col(18) << "TYPE" << col(7) << "QTY" << col(10) << "KG" << col(6) << "PRIO" << '\n';
    return os << tableRule;
}

inline void printManifestTable(std::ostream& os, const Inventory::Manifest& m) {
    std::ios_base::fmtflags f = os.flags();
    std::streamsize p = os.precision();
    os << "Manifest " << m.id() << " (" << m.size() << " lines)\n" << cargoHeader << std::fixed << std::setprecision(1);
    for (std::size_t i = 0; i < m.size(); ++i) {
        const Inventory::ReliefCargo& c = m.at(i);
        os << col(18) << c.name() << col(18) << Inventory::toString(c.type()) << col(7) << c.quantity()
           << col(10) << c.totalWeightKg() << col(6) << c.priority() << '\n';
    }
    os << tableRule << col(43) << "TOTAL" << m.totalWeightKg() << " kg\n";
    os.flags(f);
    os.precision(p);
}

}  // namespace Util
}  // namespace ReliefOps
