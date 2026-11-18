// Inventory.h - namespace ReliefOps::Inventory
// Unit I : namespaces, function overloading, call/return by reference, friend class
// Unit II: ADT / information hiding, static members, dynamic memory, operator overloading
#pragma once
#include <cstddef>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include "Exceptions.h"

namespace ReliefOps {
class IncidentCommander;  // friend class (defined in Logistics.h)

namespace Inventory {

enum class SupplyType { RationKit = 0, TraumaMedicine = 1, WaterTank = 2, ThermalBlanket = 3 };
enum class TriageLevel { Immediate, Delayed, Minor };
const int SUPPLY_TYPE_COUNT = 4;

const char* toString(SupplyType t);
const char* toString(TriageLevel t);

// ---------------------------------------------------------------- ReliefCargo
class ReliefCargo {
public:
    ReliefCargo();
    ReliefCargo(const std::string& name, SupplyType type, int quantity, double unitWeightKg, int priority);

    const std::string& name() const { return name_; }
    SupplyType type() const { return type_; }
    int quantity() const { return quantity_; }
    double unitWeightKg() const { return unitWeightKg_; }
    int priority() const { return priority_; }                       // 1 = most urgent
    double totalWeightKg() const { return quantity_ * unitWeightKg_; }  // inline

    // Duplicate-cargo check: same item name and supply type.
    bool operator==(const ReliefCargo& o) const { return name_ == o.name_ && type_ == o.type_; }
    bool operator!=(const ReliefCargo& o) const { return !(*this == o); }

private:
    std::string name_;
    SupplyType type_;
    int quantity_;
    double unitWeightKg_;
    int priority_;

    friend class ReliefOps::IncidentCommander;
    friend std::ostream& operator<<(std::ostream&, const ReliefCargo&);
    friend std::istream& operator>>(std::istream&, ReliefCargo&);
};

std::ostream& operator<<(std::ostream& os, const ReliefCargo& c);
std::istream& operator>>(std::istream& is, ReliefCargo& c);

// ----------------------------------------------------------- DistributionDepot
class DistributionDepot {
public:
    explicit DistributionDepot(const std::string& name);
    ~DistributionDepot();
    DistributionDepot(const DistributionDepot&) = delete;
    DistributionDepot& operator=(const DistributionDepot&) = delete;

    DistributionDepot& operator+=(const ReliefCargo& c);  // restock
    DistributionDepot& operator-=(const ReliefCargo& c);  // deduct on departure

    int& stockRef(SupplyType t) { return stock_[static_cast<int>(t)]; }  // return by reference
    int stockOf(SupplyType t) const;
    const std::string& name() const { return name_; }

    static int activeDepots() { return activeDepots_; }

private:
    std::string name_;
    std::unordered_map<int, int> stock_;
    static int activeDepots_;

    friend class ReliefOps::IncidentCommander;
    friend std::ostream& operator<<(std::ostream&, const DistributionDepot&);
};

std::ostream& operator<<(std::ostream& os, const DistributionDepot& d);

// Unit I: function overloading - two allocation strategies, depot passed by reference.
// (a) volume based: how many units fit in a payload of payloadKg
int allocateSupply(DistributionDepot& depot, SupplyType type, double payloadKg, double unitWeightKg);
// (b) priority based: share of stock released according to medical triage level
int allocateSupply(DistributionDepot& depot, SupplyType type, TriageLevel level);

// ------------------------------------------------------------------- Manifest
// Owns a dynamically allocated array of ReliefCargo (Rule of Three, copy-and-swap).
class Manifest {
public:
    Manifest();                                                         // default
    explicit Manifest(const std::string& id, std::size_t capacity = 4);  // parameterised + dynamic ctor
    Manifest(const Manifest& other);                                    // deep copy
    Manifest& operator=(Manifest other);                                // copy-and-swap
    ~Manifest();                                                        // frees the array

    bool add(const ReliefCargo& c);          // false if duplicate cargo
    bool contains(const ReliefCargo& c) const;
    double totalWeightKg() const;
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return capacity_; }
    const std::string& id() const { return id_; }
    const ReliefCargo& at(std::size_t i) const;

    bool operator==(const Manifest& o) const;  // identity check
    bool operator!=(const Manifest& o) const { return !(*this == o); }

    void swap(Manifest& o) noexcept;
    static int liveManifests() { return liveManifests_; }

private:
    void grow();
    std::string id_;
    ReliefCargo* items_;
    std::size_t count_;
    std::size_t capacity_;
    static int liveManifests_;

    friend std::ostream& operator<<(std::ostream&, const Manifest&);
    friend std::istream& operator>>(std::istream&, Manifest&);
};

std::ostream& operator<<(std::ostream& os, const Manifest& m);
std::istream& operator>>(std::istream& is, Manifest& m);

}  // namespace Inventory
}  // namespace ReliefOps
