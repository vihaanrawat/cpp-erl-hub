// ERL-HUB : Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
// CIC-211 Object-Oriented Programming Using C++  -  Option 2
// Demonstration driver: each section is tagged with the syllabus unit it exercises.
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Exceptions.h"
#include "Inventory.h"
#include "Logistics.h"
#include "Persistence.h"
#include "Utilities.h"

using namespace ReliefOps;
using namespace ReliefOps::Inventory;
using namespace ReliefOps::Logistics;
using ReliefOps::Util::col;

static void banner(const std::string& title) {
    std::cout << "\n==================== " << title << " ====================\n";
}

int main() {
    try {
        // ------------------------------------------------------------------ UNIT I / II
        banner("UNIT I/II: Depot, operators +=, -=, overloading, references");
        DistributionDepot depot("Srinagar-Hub");
        depot += ReliefCargo("Ration Kit", SupplyType::RationKit, 500, 2.5, 3);
        depot += ReliefCargo("Trauma Kit", SupplyType::TraumaMedicine, 200, 1.2, 1);
        depot += ReliefCargo("Water Tank", SupplyType::WaterTank, 40, 100.0, 2);
        depot += ReliefCargo("Thermal Blanket", SupplyType::ThermalBlanket, 800, 0.6, 4);
        std::cout << depot;

        int rations = allocateSupply(depot, SupplyType::RationKit, 400.0, 2.5);            // overload (a)
        int trauma = allocateSupply(depot, SupplyType::TraumaMedicine, TriageLevel::Immediate);  // overload (b)
        std::cout << "Volume-based allocation : " << rations << " ration kits for a 400 kg payload\n";
        std::cout << "Triage-based allocation : " << trauma << " trauma kits released (IMMEDIATE)\n";
        depot.stockRef(SupplyType::WaterTank) += 10;  // return-by-reference used as an l-value
        std::cout << "After updates -> Water Tanks: " << depot.stockOf(SupplyType::WaterTank)
                  << ", Trauma: " << depot.stockOf(SupplyType::TraumaMedicine) << '\n';

        try {
            depot -= ReliefCargo("Ration Kit", SupplyType::RationKit, 9999, 2.5, 3);
        } catch (const StockShortageException& e) {
            std::cout << "Caught: " << e.what() << '\n';
        }

        // ------------------------------------------------------------ Dynamic memory
        banner("UNIT II: Manifest (dynamic memory, deep copy, ==, <<, >>)");
        Manifest m1("MF-001", 2);  // capacity 2 -> will grow automatically
        m1.add(ReliefCargo("Trauma Kit", SupplyType::TraumaMedicine, 50, 1.2, 1));
        m1.add(ReliefCargo("Ration Kit", SupplyType::RationKit, 160, 2.5, 3));
        m1.add(ReliefCargo("Thermal Blanket", SupplyType::ThermalBlanket, 300, 0.6, 4));
        std::cout << "Capacity grew 2 -> " << m1.capacity() << " for " << m1.size() << " lines\n";
        std::cout << "Duplicate cargo accepted? " << std::boolalpha
                  << m1.add(ReliefCargo("Trauma Kit", SupplyType::TraumaMedicine, 5, 1.2, 1)) << '\n';

        Manifest m2 = m1;  // deep copy constructor
        m2.add(ReliefCargo("Rope Bundle", SupplyType::RationKit, 10, 3.0, 5));
        std::cout << "Original has " << m1.size() << " lines, copy has " << m2.size() << " (independent memory)\n";
        std::cout << "m1 == m2 ? " << (m1 == m2) << "   m1 == copy-of-m1 ? " << (m1 == Manifest(m1)) << '\n';

        std::stringstream wire;  // serialization through << and >>
        wire << m1;
        Manifest m4;
        wire >> m4;
        std::cout << "Round-trip through << / >> preserves manifest? " << (m1 == m4) << '\n';
        Util::printManifestTable(std::cout, m1);
        std::cout << "Live manifests (static counter): " << Manifest::liveManifests() << '\n';

        // ---------------------------------------------------------------- UNIT III
        banner("UNIT III: Fleet through ReliefVehicle* (virtual base, polymorphism)");
        std::vector<ReliefVehicle*> fleet;
        fleet.push_back(new AerialTransport("MedEvac-Heli-1", 800.0, 600.0, 220.0));
        fleet.push_back(new GroundConvoy("Truck-Alpha", 5000.0, 450.0, 45.0));
        fleet.push_back(new AmphibiousCraft("Boat-Delta", 1200.0, 300.0, 35.0));
        fleet.push_back(new AmphibiousMedicalUnit("AMU-Medic-1", 2000.0, 400.0, 40.0));
        std::cout << "Operational vehicles (static): " << ReliefVehicle::operationalVehicles() << '\n';
        std::cout << "AMU is a single ReliefVehicle through both parents (virtual base resolves the diamond)\n";

        Corridor corridors[] = {
            {"NH-44 Banihal Pass", 120.0, false, false, 74.9, 33.5},
            {"Jhelum Flood Road", 80.0, true, true, 74.8, 34.0},
            {"Landslide Ridge Track", 150.0, true, false, 75.1, 33.9}};

        std::cout << std::fixed << std::setprecision(2);
        for (std::size_t v = 0; v < fleet.size(); ++v) {
            for (std::size_t c = 0; c < 3; ++c) {
                std::cout << col(16) << fleet[v]->name() << col(22) << fleet[v]->type() << col(24) << corridors[c].name;
                try {
                    std::cout << "ETA " << fleet[v]->evaluateRoute(corridors[c]) << " h\n";  // dynamic dispatch
                } catch (const std::exception& e) {
                    std::cout << "BLOCKED - " << e.what() << '\n';
                }
            }
        }

        banner("UNIT III/IV: Dispatch, exceptions, IncidentCommander (friend)");
        std::vector<Persistence::TransitLogRecord> transitLog;
        double eta = fleet[0]->dispatch(corridors[0], m1);
        std::cout << fleet[0]->name() << " dispatched, ETA " << eta << " h, utilisation "
                  << payloadUtilisation(fleet[0]->loadKg(), fleet[0]->capacityKg()) << "%, life-support "
                  << fleet[0]->lifeSupport().status() << ", GPS pings " << fleet[0]->gps().pings() << '\n';
        Persistence::TransitLogRecord r1 = {m1.id(), fleet[0]->name(), corridors[0].name, corridors[0].distanceKm,
                                            m1.totalWeightKg() / 1000.0, eta};
        transitLog.push_back(r1);

        eta = fleet[3]->dispatch(corridors[1], m1);  // road damaged -> hybrid switches to waterway
        std::cout << fleet[3]->name() << " used the waterway fallback, ETA " << eta << " h\n";
        Persistence::TransitLogRecord r2 = {m1.id(), fleet[3]->name(), corridors[1].name, corridors[1].distanceKm,
                                            m1.totalWeightKg() / 1000.0, eta};
        transitLog.push_back(r2);

        Manifest heavy("MF-HEAVY", 1);
        heavy.add(ReliefCargo("Water Tank", SupplyType::WaterTank, 20, 100.0, 2));  // 2000 kg
        try {
            fleet[0]->dispatch(corridors[0], heavy);
        } catch (const CargoOverweightException& e) {
            std::cout << "Caught: " << e.what() << " | excess " << e.excessKg() << " kg\n";
        }

        Corridor longHaul = {"Cross-Valley Express", 400.0, false, true, 76.0, 34.5};
        try {
            fleet[2]->dispatch(longHaul, m1);
        } catch (const FuelDepletionException& e) {
            std::cout << "Caught: " << e.what() << '\n';
        }

        IncidentCommander cmdr("Alpha-Actual");
        cmdr.forceLoad(*fleet[0], heavy);  // friend class overrides the capacity barrier
        cmdr.emergencyRefuel(*fleet[2], 200.0);
        cmdr.requisition(depot, SupplyType::WaterTank, 25);
        std::cout << cmdr.callsign() << " forced " << fleet[0]->loadKg() << " kg onto " << fleet[0]->name()
                  << " (critical override), refuelled " << fleet[2]->name() << " to " << fleet[2]->fuelRangeKm()
                  << " km, depot water tanks now " << depot.stockOf(SupplyType::WaterTank) << '\n';

        std::cout << "Total dispatched tonnage (static): " << ReliefVehicle::totalDispatchedTonnes() << " t\n";

        // ----------------------------------------------------------------- UNIT IV
        banner("UNIT IV: Template queue, functors, STL algorithms, manipulators");
        std::vector<ReliefCargo> requests;
        requests.push_back(ReliefCargo("Thermal Blanket", SupplyType::ThermalBlanket, 300, 0.6, 4));
        requests.push_back(ReliefCargo("Trauma Kit", SupplyType::TraumaMedicine, 80, 1.2, 1));
        requests.push_back(ReliefCargo("Water Tank", SupplyType::WaterTank, 6, 100.0, 2));
        requests.push_back(ReliefCargo("Ration Kit", SupplyType::RationKit, 200, 2.5, 3));
        requests.push_back(ReliefCargo("Burn Dressing", SupplyType::TraumaMedicine, 120, 0.3, 1));

        Util::PriorityDispatchQueue<ReliefCargo, Util::MoreUrgentFirst> queue;
        std::for_each(requests.begin(), requests.end(), [&queue](const ReliefCargo& c) { queue.push(c); });
        std::cout << "Dispatch order from PriorityDispatchQueue<ReliefCargo>:\n";
        while (!queue.empty()) {
            ReliefCargo c = queue.pop();
            std::cout << "  P" << c.priority() << "  " << col(16) << c.name() << c.quantity() << " units\n";
        }

        Util::PriorityDispatchQueue<int> smallestFirst;  // same template, different T
        smallestFirst.push(42); smallestFirst.push(7); smallestFirst.push(19);
        std::cout << "PriorityDispatchQueue<int> pops: " << smallestFirst.pop() << ' ' << smallestFirst.pop() << ' '
                  << smallestFirst.pop() << '\n';

        std::sort(requests.begin(), requests.end(), Util::MoreUrgentFirst());
        std::unordered_map<std::string, double> weightByType;
        std::for_each(requests.begin(), requests.end(),
                      [&weightByType](const ReliefCargo& c) { weightByType[toString(c.type())] += c.totalWeightKg(); });
        std::cout << "Weight per supply type (unordered_map):\n";
        std::cout << std::fixed << std::setprecision(1);
        for (std::unordered_map<std::string, double>::const_iterator it = weightByType.begin(); it != weightByType.end(); ++it)
            std::cout << "  " << col(18) << it->first << it->second << " kg\n";

        banner("UNIT IV: Binary persistence, atomic update, corruption detection");
        transitLog.push_back(Persistence::TransitLogRecord{"MF-HEAVY", "MedEvac-Heli-1", "NH-44 Banihal Pass", 120.0, 2.0, 0.55});
        Persistence::saveTransitLog("transit_log.bin", transitLog);
        Persistence::saveManifest("manifest_MF-001.bin", m1);
        std::vector<Persistence::TransitLogRecord> restored = Persistence::loadTransitLog("transit_log.bin");
        std::cout << "Restored " << restored.size() << " transit records:\n" << Util::tableRule;
        for (std::size_t i = 0; i < restored.size(); ++i)
            std::cout << col(10) << restored[i].manifestId << col(18) << restored[i].vehicle << col(24)
                      << restored[i].corridor << restored[i].tonnage << " t\n";
        Manifest back = Persistence::loadManifest("manifest_MF-001.bin");
        std::cout << "Manifest restored from disk identical to original? " << std::boolalpha << (back == m1) << '\n';

        {   // flip one byte inside the file and prove the checksum catches it
            std::fstream f("transit_log.bin", std::ios::in | std::ios::out | std::ios::binary);
            char ch;
            f.seekg(10);
            f.get(ch);
            ch = static_cast<char>(ch ^ 0xFF);
            f.seekp(10);
            f.put(ch);
        }
        try {
            Persistence::loadTransitLog("transit_log.bin");
        } catch (const PersistenceException& e) {
            std::cout << "Caught: " << e.what() << '\n';
        }

        // ------------------------------------------------------------ virtual dtors
        banner("UNIT III: Fleet deallocation through base pointers (virtual destructors)");
        for (std::size_t i = 0; i < fleet.size(); ++i) delete fleet[i];
        fleet.clear();
        std::cout << "Operational vehicles after cleanup: " << ReliefVehicle::operationalVehicles() << '\n';
        std::cout << "Active depots: " << DistributionDepot::activeDepots() << "  (depot still in scope)\n";

        std::remove("transit_log.bin");
        std::remove("manifest_MF-001.bin");
    } catch (const std::exception& e) {
        std::cerr << "Unhandled error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
