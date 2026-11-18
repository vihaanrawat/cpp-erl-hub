// Persistence.h - Unit IV: binary std::fstream persistence with atomic (temp file + rename) updates
#pragma once
#include <string>
#include <vector>
#include "Inventory.h"

namespace ReliefOps {
namespace Persistence {

struct TransitLogRecord {
    std::string manifestId;
    std::string vehicle;
    std::string corridor;
    double distanceKm;
    double tonnage;
    double etaHours;
};

// Writes to "<path>.tmp", verifies the stream, then renames over <path>:
// a crash mid-write can never leave a half-written log behind.
void saveTransitLog(const std::string& path, const std::vector<TransitLogRecord>& records);
std::vector<TransitLogRecord> loadTransitLog(const std::string& path);  // verifies checksum

void saveManifest(const std::string& path, const Inventory::Manifest& m);
Inventory::Manifest loadManifest(const std::string& path);

}  // namespace Persistence
}  // namespace ReliefOps
