// Logistics.h - namespace ReliefOps::Logistics
// Unit I : inline utilities, friend class IncidentCommander
// Unit II: static members (fleet analytics)
// Unit III: abstract base, virtual-base diamond, containership, runtime polymorphism
#pragma once
#include <cmath>
#include <string>
#include "Exceptions.h"
#include "Inventory.h"

namespace ReliefOps {
class IncidentCommander;

namespace Logistics {

// ---- inline utilities for rapid distance / payload calculations
inline double euclideanKm(double x1, double y1, double x2, double y2) {
    return std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}
inline double payloadUtilisation(double loadKg, double capacityKg) {
    return capacityKg > 0.0 ? 100.0 * loadKg / capacityKg : 0.0;
}

struct Corridor {
    std::string name;
    double distanceKm;
    bool damaged;    // road/airstrip damaged by the quake
    bool waterway;   // navigable by boat
    double toX, toY; // destination coordinates
};

// ---- Contained (HAS-A) components - composed, NOT inherited
class GPSBeacon {
public:
    explicit GPSBeacon(const std::string& id = "GPS-UNSET", double x = 0.0, double y = 0.0)
        : id_(id), x_(x), y_(y), pings_(0) {}
    void update(double x, double y) { x_ = x; y_ = y; ++pings_; }
    double x() const { return x_; }
    double y() const { return y_; }
    int pings() const { return pings_; }
    const std::string& id() const { return id_; }
private:
    std::string id_;
    double x_, y_;
    int pings_;
};

class EmergencyLifeSupportUnit {
public:
    explicit EmergencyLifeSupportUnit(int oxygenMinutes = 240) : oxygenMinutes_(oxygenMinutes), active_(false) {}
    void activate() { active_ = true; }
    bool active() const { return active_; }
    int oxygenMinutes() const { return oxygenMinutes_; }
    std::string status() const { return active_ ? "ACTIVE" : "STANDBY"; }
private:
    int oxygenMinutes_;
    bool active_;
};

// ---- Abstract root of the hierarchy
class ReliefVehicle {
public:
    ReliefVehicle(const std::string& name, double capacityKg, double fuelRangeKm, double speedKmh);
    virtual ~ReliefVehicle();  // virtual: safe delete through ReliefVehicle*
    ReliefVehicle(const ReliefVehicle&) = delete;
    ReliefVehicle& operator=(const ReliefVehicle&) = delete;

    virtual std::string type() const = 0;
    virtual double evaluateRoute(const Corridor& c) const = 0;  // ETA in hours, or throws
    virtual double fuelBurnFactor() const { return 1.0; }

    // Validates weight, evaluates route (virtual call), burns fuel, updates analytics. Returns ETA.
    double dispatch(const Corridor& c, const Inventory::Manifest& m);

    const std::string& name() const { return name_; }
    double capacityKg() const { return capacityKg_; }
    double loadKg() const { return loadKg_; }
    double fuelRangeKm() const { return fuelRangeKm_; }
    double speedKmh() const { return speedKmh_; }
    GPSBeacon& gps() { return gps_; }  // return by reference
    const GPSBeacon& gps() const { return gps_; }
    const EmergencyLifeSupportUnit& lifeSupport() const { return lifeSupport_; }

    static int operationalVehicles() { return operationalVehicles_; }
    static double totalDispatchedTonnes() { return totalDispatchedTonnes_; }

protected:
    double checkedEta(const Corridor& c, double effectiveSpeedKmh) const;  // fuel check + ETA

private:
    std::string name_;
    double capacityKg_;
    double fuelRangeKm_;
    double speedKmh_;
    double loadKg_;
    GPSBeacon gps_;                       // containership
    EmergencyLifeSupportUnit lifeSupport_; // containership

    static int operationalVehicles_;
    static double totalDispatchedTonnes_;

    friend class ReliefOps::IncidentCommander;
};

// ---- Intermediate classes share ONE ReliefVehicle via virtual inheritance
class AerialTransport : public virtual ReliefVehicle {
public:
    AerialTransport(const std::string& n, double cap, double range, double speed) : ReliefVehicle(n, cap, range, speed) {}
    ~AerialTransport() override;
    std::string type() const override { return "AerialTransport"; }
    double fuelBurnFactor() const override { return 2.0; }
    double evaluateRoute(const Corridor& c) const override;  // flies over damage
};

class GroundConvoy : public virtual ReliefVehicle {
public:
    GroundConvoy(const std::string& n, double cap, double range, double speed) : ReliefVehicle(n, cap, range, speed) {}
    ~GroundConvoy() override;
    std::string type() const override { return "GroundConvoy"; }
    double evaluateRoute(const Corridor& c) const override;  // blocked by damaged corridors
};

class AmphibiousCraft : public virtual ReliefVehicle {
public:
    AmphibiousCraft(const std::string& n, double cap, double range, double speed) : ReliefVehicle(n, cap, range, speed) {}
    ~AmphibiousCraft() override;
    std::string type() const override { return "AmphibiousCraft"; }
    double fuelBurnFactor() const override { return 1.5; }
    double evaluateRoute(const Corridor& c) const override;  // water fast, land slow
};

// ---- Diamond:   ReliefVehicle
//               /               \   (virtual)
//        GroundConvoy      AmphibiousCraft
//               \               /
//             AmphibiousMedicalUnit
class AmphibiousMedicalUnit : public GroundConvoy, public AmphibiousCraft {
public:
    AmphibiousMedicalUnit(const std::string& n, double cap, double range, double speed)
        : ReliefVehicle(n, cap, range, speed),   // most-derived class initialises the virtual base
          GroundConvoy(n, cap, range, speed),
          AmphibiousCraft(n, cap, range, speed) {}
    ~AmphibiousMedicalUnit() override;
    std::string type() const override { return "AmphibiousMedicalUnit"; }
    double fuelBurnFactor() const override { return 1.25; }
    double evaluateRoute(const Corridor& c) const override;  // road first, waterway fallback
};

}  // namespace Logistics

// ---- Friend class: may bypass access barriers during critical triage overrides
class IncidentCommander {
public:
    explicit IncidentCommander(const std::string& callsign) : callsign_(callsign) {}
    const std::string& callsign() const { return callsign_; }

    void forceLoad(Logistics::ReliefVehicle& v, const Inventory::Manifest& m);   // ignores capacity
    void emergencyRefuel(Logistics::ReliefVehicle& v, double km);
    void requisition(Inventory::DistributionDepot& d, Inventory::SupplyType t, int qty);
    void reprioritise(Inventory::ReliefCargo& c, int newPriority);
private:
    std::string callsign_;
};

}  // namespace ReliefOps
