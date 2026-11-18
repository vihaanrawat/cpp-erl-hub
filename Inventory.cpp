#include "Inventory.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace ReliefOps {
namespace Inventory {

const char* toString(SupplyType t) {
    switch (t) {
        case SupplyType::RationKit: return "Ration Kit";
        case SupplyType::TraumaMedicine: return "Trauma Medicine";
        case SupplyType::WaterTank: return "Water Tank";
        case SupplyType::ThermalBlanket: return "Thermal Blanket";
    }
    return "Unknown";
}

const char* toString(TriageLevel t) {
    switch (t) {
        case TriageLevel::Immediate: return "IMMEDIATE";
        case TriageLevel::Delayed: return "DELAYED";
        case TriageLevel::Minor: return "MINOR";
    }
    return "Unknown";
}

static std::string noSpaces(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i)
        if (s[i] == ' ') s[i] = '_';
    return s;
}

// ---------------------------------------------------------------- ReliefCargo
ReliefCargo::ReliefCargo()
    : name_("UNNAMED"), type_(SupplyType::RationKit), quantity_(0), unitWeightKg_(0.0), priority_(5) {}

ReliefCargo::ReliefCargo(const std::string& name, SupplyType type, int quantity, double unitWeightKg, int priority)
    : name_(noSpaces(name)), type_(type), quantity_(quantity), unitWeightKg_(unitWeightKg), priority_(priority) {
    if (quantity < 0 || unitWeightKg < 0.0)
        throw std::invalid_argument("ReliefCargo: quantity and weight must be non-negative");
}

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c) {
    return os << c.name_ << ' ' << static_cast<int>(c.type_) << ' ' << c.quantity_ << ' '
              << c.unitWeightKg_ << ' ' << c.priority_;
}

std::istream& operator>>(std::istream& is, ReliefCargo& c) {
    std::string name;
    int type, qty, prio;
    double w;
    if (!(is >> name >> type >> qty >> w >> prio)) return is;
    if (type < 0 || type >= SUPPLY_TYPE_COUNT || qty < 0 || w < 0.0) {
        is.setstate(std::ios::failbit);
        return is;
    }
    c = ReliefCargo(name, static_cast<SupplyType>(type), qty, w, prio);
    return is;
}

// ----------------------------------------------------------- DistributionDepot
int DistributionDepot::activeDepots_ = 0;

DistributionDepot::DistributionDepot(const std::string& name) : name_(name) { ++activeDepots_; }
DistributionDepot::~DistributionDepot() { --activeDepots_; }

int DistributionDepot::stockOf(SupplyType t) const {
    std::unordered_map<int, int>::const_iterator it = stock_.find(static_cast<int>(t));
    return it == stock_.end() ? 0 : it->second;
}

DistributionDepot& DistributionDepot::operator+=(const ReliefCargo& c) {
    if (c.quantity() <= 0) throw std::invalid_argument("restock quantity must be positive");
    stock_[static_cast<int>(c.type())] += c.quantity();
    return *this;
}

DistributionDepot& DistributionDepot::operator-=(const ReliefCargo& c) {
    if (c.quantity() <= 0) return *this;
    int available = stockOf(c.type());
    if (c.quantity() > available)
        throw StockShortageException(name_, toString(c.type()), c.quantity(), available);
    stock_[static_cast<int>(c.type())] -= c.quantity();
    return *this;
}

std::ostream& operator<<(std::ostream& os, const DistributionDepot& d) {
    os << "Depot [" << d.name_ << "]\n";
    for (int t = 0; t < SUPPLY_TYPE_COUNT; ++t)
        os << "   " << toString(static_cast<SupplyType>(t)) << ": " << d.stockOf(static_cast<SupplyType>(t)) << '\n';
    return os;
}

int allocateSupply(DistributionDepot& depot, SupplyType type, double payloadKg, double unitWeightKg) {
    if (payloadKg <= 0.0 || unitWeightKg <= 0.0) return 0;
    int fits = static_cast<int>(payloadKg / unitWeightKg);
    int& stock = depot.stockRef(type);  // reference: modify the depot's stock in place
    int give = std::min(fits, stock);
    stock -= give;
    return give;
}

int allocateSupply(DistributionDepot& depot, SupplyType type, TriageLevel level) {
    int& stock = depot.stockRef(type);
    double share = (level == TriageLevel::Immediate) ? 0.60 : (level == TriageLevel::Delayed ? 0.30 : 0.10);
    int give = static_cast<int>(stock * share);
    if (give == 0 && stock > 0) give = 1;
    stock -= give;
    return give;
}

// ------------------------------------------------------------------- Manifest
int Manifest::liveManifests_ = 0;

Manifest::Manifest() : id_("UNASSIGNED"), items_(nullptr), count_(0), capacity_(0) { ++liveManifests_; }

Manifest::Manifest(const std::string& id, std::size_t capacity)
    : id_(noSpaces(id)), items_(capacity ? new ReliefCargo[capacity] : nullptr), count_(0), capacity_(capacity) {
    ++liveManifests_;
}

Manifest::Manifest(const Manifest& o)
    : id_(o.id_), items_(o.capacity_ ? new ReliefCargo[o.capacity_] : nullptr), count_(o.count_), capacity_(o.capacity_) {
    for (std::size_t i = 0; i < count_; ++i) items_[i] = o.items_[i];
    ++liveManifests_;
}

Manifest& Manifest::operator=(Manifest other) {
    swap(other);
    return *this;
}

Manifest::~Manifest() {
    delete[] items_;
    --liveManifests_;
}

void Manifest::swap(Manifest& o) noexcept {
    std::swap(id_, o.id_);
    std::swap(items_, o.items_);
    std::swap(count_, o.count_);
    std::swap(capacity_, o.capacity_);
}

void Manifest::grow() {
    std::size_t newCap = capacity_ ? capacity_ * 2 : 4;
    ReliefCargo* fresh = new ReliefCargo[newCap];
    try {
        for (std::size_t i = 0; i < count_; ++i) fresh[i] = items_[i];
    } catch (...) {
        delete[] fresh;
        throw;
    }
    delete[] items_;
    items_ = fresh;
    capacity_ = newCap;
}

bool Manifest::contains(const ReliefCargo& c) const {
    for (std::size_t i = 0; i < count_; ++i)
        if (items_[i] == c) return true;
    return false;
}

bool Manifest::add(const ReliefCargo& c) {
    if (contains(c)) return false;
    if (count_ == capacity_) grow();
    items_[count_++] = c;
    return true;
}

double Manifest::totalWeightKg() const {
    double sum = 0.0;
    for (std::size_t i = 0; i < count_; ++i) sum += items_[i].totalWeightKg();
    return sum;
}

const ReliefCargo& Manifest::at(std::size_t i) const {
    if (i >= count_) throw std::out_of_range("Manifest::at index out of range");
    return items_[i];
}

bool Manifest::operator==(const Manifest& o) const {
    if (id_ != o.id_ || count_ != o.count_) return false;
    for (std::size_t i = 0; i < count_; ++i)
        if (!(items_[i] == o.items_[i]) || items_[i].quantity() != o.items_[i].quantity()) return false;
    return true;
}

std::ostream& operator<<(std::ostream& os, const Manifest& m) {
    os << "MANIFEST " << m.id_ << ' ' << m.count_ << '\n';
    for (std::size_t i = 0; i < m.count_; ++i) os << m.items_[i] << '\n';
    return os;
}

std::istream& operator>>(std::istream& is, Manifest& m) {
    std::string tag, id;
    std::size_t n = 0;
    if (!(is >> tag >> id >> n)) return is;
    if (tag != "MANIFEST" || n > 100000) {
        is.setstate(std::ios::failbit);
        return is;
    }
    Manifest tmp(id, n);
    for (std::size_t i = 0; i < n; ++i) {
        ReliefCargo c;
        if (!(is >> c)) return is;
        if (!tmp.add(c)) {
            is.setstate(std::ios::failbit);
            return is;
        }
    }
    m.swap(tmp);
    return is;
}

}  // namespace Inventory
}  // namespace ReliefOps
