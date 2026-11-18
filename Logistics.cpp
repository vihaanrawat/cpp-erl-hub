#include "Logistics.h"
#include <iostream>
#include <stdexcept>

namespace ReliefOps {
namespace Logistics {

int ReliefVehicle::operationalVehicles_ = 0;
double ReliefVehicle::totalDispatchedTonnes_ = 0.0;

ReliefVehicle::ReliefVehicle(const std::string& name, double capacityKg, double fuelRangeKm, double speedKmh)
    : name_(name), capacityKg_(capacityKg), fuelRangeKm_(fuelRangeKm), speedKmh_(speedKmh), loadKg_(0.0),
      gps_(name + "-GPS"), lifeSupport_() {
    if (capacityKg <= 0.0 || fuelRangeKm <= 0.0 || speedKmh <= 0.0)
        throw std::invalid_argument("ReliefVehicle: capacity, range and speed must be positive");
    ++operationalVehicles_;
}

ReliefVehicle::~ReliefVehicle() {
    --operationalVehicles_;
    std::cout << "  [dtor] ReliefVehicle base part of " << name_ << '\n';
}

double ReliefVehicle::checkedEta(const Corridor& c, double effectiveSpeedKmh) const {
    double needed = c.distanceKm * fuelBurnFactor();  // virtual call
    if (needed > fuelRangeKm_) throw FuelDepletionException(name_, needed, fuelRangeKm_);
    return c.distanceKm / effectiveSpeedKmh;
}

double ReliefVehicle::dispatch(const Corridor& c, const Inventory::Manifest& m) {
    double weight = m.totalWeightKg();
    if (weight > capacityKg_) throw CargoOverweightException(name_, weight, capacityKg_);
    double eta = evaluateRoute(c);  // runtime polymorphism
    loadKg_ = weight;
    fuelRangeKm_ -= c.distanceKm * fuelBurnFactor();
    gps_.update(c.toX, c.toY);
    for (std::size_t i = 0; i < m.size(); ++i)
        if (m.at(i).type() == Inventory::SupplyType::TraumaMedicine) lifeSupport_.activate();
    totalDispatchedTonnes_ += weight / 1000.0;
    return eta;
}

AerialTransport::~AerialTransport() { std::cout << "  [dtor] AerialTransport part of " << name() << '\n'; }
GroundConvoy::~GroundConvoy() { std::cout << "  [dtor] GroundConvoy part of " << name() << '\n'; }
AmphibiousCraft::~AmphibiousCraft() { std::cout << "  [dtor] AmphibiousCraft part of " << name() << '\n'; }
AmphibiousMedicalUnit::~AmphibiousMedicalUnit() { std::cout << "  [dtor] AmphibiousMedicalUnit " << name() << '\n'; }

double AerialTransport::evaluateRoute(const Corridor& c) const { return checkedEta(c, speedKmh()); }

double GroundConvoy::evaluateRoute(const Corridor& c) const {
    if (c.damaged) throw DamagedCorridorException(c.name);
    return checkedEta(c, speedKmh());
}

double AmphibiousCraft::evaluateRoute(const Corridor& c) const {
    if (c.damaged && !c.waterway) throw DamagedCorridorException(c.name);
    return checkedEta(c, c.waterway ? speedKmh() : speedKmh() * 0.5);
}

double AmphibiousMedicalUnit::evaluateRoute(const Corridor& c) const {
    try {
        return GroundConvoy::evaluateRoute(c);        // try the road first
    } catch (const DamagedCorridorException&) {
        return AmphibiousCraft::evaluateRoute(c);     // fall back to the waterway
    }
}

}  // namespace Logistics

// ------------------------------------------------------------ IncidentCommander
void IncidentCommander::forceLoad(Logistics::ReliefVehicle& v, const Inventory::Manifest& m) {
    v.loadKg_ = m.totalWeightKg();  // direct private access (friend)
    Logistics::ReliefVehicle::totalDispatchedTonnes_ += v.loadKg_ / 1000.0;
}

void IncidentCommander::emergencyRefuel(Logistics::ReliefVehicle& v, double km) { v.fuelRangeKm_ += km; }

void IncidentCommander::requisition(Inventory::DistributionDepot& d, Inventory::SupplyType t, int qty) {
    d.stock_[static_cast<int>(t)] += qty;
}

void IncidentCommander::reprioritise(Inventory::ReliefCargo& c, int newPriority) { c.priority_ = newPriority; }

}  // namespace ReliefOps
