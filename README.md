# ERL-HUB - Post-Earthquake Multi-Agency Emergency Relief Logistics Hub
CIC-211 Object-Oriented Programming Using C++ - Project Option 2

## Build & run
    make run            # or: g++ -std=c++14 main.cpp Inventory.cpp Logistics.cpp Persistence.cpp -o erl_hub
Compiles warning-free with -std=c++11/14/17 (-Wall -Wextra -pedantic); clean under AddressSanitizer/UBSan.

## Files
| File | Purpose |
|---|---|
| Exceptions.h | Custom exception hierarchy (all derive from std::exception) |
| Inventory.h/.cpp | `ReliefOps::Inventory` - ReliefCargo, DistributionDepot, Manifest, allocation overloads |
| Logistics.h/.cpp | `ReliefOps::Logistics` - vehicles, GPSBeacon, life-support unit, IncidentCommander |
| Utilities.h | `PriorityDispatchQueue<T>`, functors, I/O manipulators |
| Persistence.h/.cpp | Binary fstream save/load with atomic update + checksum |
| main.cpp | Demo driver, sections labelled by unit |

## Requirement map
### Unit I
- Namespaces `ReliefOps::Logistics`, `ReliefOps::Inventory` - Logistics.h, Inventory.h
- Function overloading `allocateSupply(...)` (volume-based vs triage-based) - Inventory.h/.cpp
- Call by reference: `allocateSupply(DistributionDepot&...)`; return by reference: `DistributionDepot::stockRef()`, `ReliefVehicle::gps()`
- Inline utilities `euclideanKm()`, `payloadUtilisation()`, `ReliefCargo::totalWeightKg()` - Logistics.h / Inventory.h
- Friend class `IncidentCommander` (forceLoad, emergencyRefuel, requisition, reprioritise) - Logistics.h/.cpp
### Unit II
- ADTs with information hiding / const correctness: ReliefCargo, DistributionDepot, ReliefVehicle
- Static data + functions: `ReliefVehicle::operationalVehicles()/totalDispatchedTonnes()`, `DistributionDepot::activeDepots()`, `Manifest::liveManifests()`
- Dynamic memory: `Manifest` (parameterised/dynamic ctor, deep copy ctor, copy-and-swap `operator=`, destructor, auto-growth)
- Operators: `+=` (restock), `-=` (deduct), `==` (cargo + manifest identity), `<<` / `>>` (cargo, depot, manifest)
### Unit III
- Base `ReliefVehicle` -> `AerialTransport`, `GroundConvoy`, `AmphibiousCraft`
- Diamond solved with virtual base: `AmphibiousMedicalUnit : GroundConvoy, AmphibiousCraft`
- Containership: `GPSBeacon` and `EmergencyLifeSupportUnit` are members of ReliefVehicle
- Runtime polymorphism via `std::vector<ReliefVehicle*>`, `evaluateRoute()`; virtual destructor chain shown on deallocation
### Unit IV
- Exceptions: CargoOverweight, FuelDepletion, DamagedCorridor (+ StockShortage, Persistence) with custom `what()`
- Binary `std::fstream` persistence, atomic update (write `.tmp` then rename), FNV-1a checksum detects corruption - Persistence.cpp
- `PriorityDispatchQueue<T, Compare>`, STL `vector`, `unordered_map`, `std::sort`, `std::for_each`, functor `MoreUrgentFirst`
- Manipulators: `col(n)`, `tableRule`, `cargoHeader` (tabular manifest printing)

## Notes
- Item names/IDs have spaces converted to `_` so text serialization (`<<`/`>>`) round-trips.
- Demo files `transit_log.bin` / `manifest_MF-001.bin` are created and deleted by main.cpp.
