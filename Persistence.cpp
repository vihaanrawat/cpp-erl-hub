#include "Persistence.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace ReliefOps {
namespace Persistence {
namespace {

typedef std::vector<char> Bytes;
const std::uint32_t kLogMagic = 0x474F4C52u;       // "RLOG"
const std::uint32_t kManifestMagic = 0x464D4C52u;  // "RLMF"

std::uint32_t checksum(const Bytes& b) {  // FNV-1a
    std::uint32_t h = 2166136261u;
    for (std::size_t i = 0; i < b.size(); ++i) {
        h ^= static_cast<unsigned char>(b[i]);
        h *= 16777619u;
    }
    return h;
}

template <typename T>
void putPod(Bytes& b, const T& v) {
    const char* p = reinterpret_cast<const char*>(&v);
    b.insert(b.end(), p, p + sizeof(T));
}

void putStr(Bytes& b, const std::string& s, std::size_t width) {
    std::string t = s.substr(0, width - 1);
    t.resize(width, '\0');
    b.insert(b.end(), t.begin(), t.end());
}

class Reader {
public:
    explicit Reader(const Bytes& b) : b_(b), pos_(0) {}
    template <typename T>
    T get() {
        need(sizeof(T));
        T v;
        std::memcpy(&v, &b_[pos_], sizeof(T));
        pos_ += sizeof(T);
        return v;
    }
    std::string str(std::size_t width) {
        need(width);
        std::string s(&b_[pos_], width);
        pos_ += width;
        return std::string(s.c_str());  // cut at first NUL
    }
private:
    void need(std::size_t n) const {
        if (pos_ + n > b_.size()) throw PersistenceException("file truncated or malformed");
    }
    const Bytes& b_;
    std::size_t pos_;
};

// File layout: [magic u32][payload size u32][payload][checksum u32]
void writeAtomic(const std::string& path, std::uint32_t magic, const Bytes& payload) {
    const std::string tmp = path + ".tmp";
    {
        std::fstream f(tmp.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!f) throw PersistenceException("cannot open " + tmp);
        std::uint32_t size = static_cast<std::uint32_t>(payload.size());
        std::uint32_t sum = checksum(payload);
        f.write(reinterpret_cast<const char*>(&magic), sizeof magic);
        f.write(reinterpret_cast<const char*>(&size), sizeof size);
        if (!payload.empty()) f.write(&payload[0], static_cast<std::streamsize>(payload.size()));
        f.write(reinterpret_cast<const char*>(&sum), sizeof sum);
        f.flush();
        if (!f) {
            f.close();
            std::remove(tmp.c_str());
            throw PersistenceException("write failed for " + tmp);
        }
    }
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {  // Windows cannot rename over an existing file
        std::remove(path.c_str());
        if (std::rename(tmp.c_str(), path.c_str()) != 0) {
            std::remove(tmp.c_str());
            throw PersistenceException("cannot replace " + path);
        }
    }
}

Bytes readVerified(const std::string& path, std::uint32_t expectedMagic) {
    std::fstream f(path.c_str(), std::ios::in | std::ios::binary);
    if (!f) throw PersistenceException("cannot open " + path);
    f.seekg(0, std::ios::end);
    std::streamoff total = f.tellg();
    f.seekg(0, std::ios::beg);
    if (total < 12) throw PersistenceException(path + " is too small to be valid");
    Bytes all(static_cast<std::size_t>(total));
    f.read(&all[0], total);
    if (!f) throw PersistenceException("read failed for " + path);

    Reader r(all);
    std::uint32_t magic = r.get<std::uint32_t>();
    std::uint32_t size = r.get<std::uint32_t>();
    if (magic != expectedMagic) throw PersistenceException(path + " has wrong file signature");
    if (all.size() != static_cast<std::size_t>(size) + 12) throw PersistenceException(path + " has inconsistent size");
    Bytes payload(all.begin() + 8, all.begin() + 8 + size);
    std::uint32_t stored;
    std::memcpy(&stored, &all[8 + size], sizeof stored);
    if (stored != checksum(payload)) throw PersistenceException("checksum mismatch in " + path + " (file corrupted)");
    return payload;
}

}  // namespace

void saveTransitLog(const std::string& path, const std::vector<TransitLogRecord>& records) {
    Bytes b;
    putPod(b, static_cast<std::uint32_t>(records.size()));
    for (std::size_t i = 0; i < records.size(); ++i) {
        putStr(b, records[i].manifestId, 24);
        putStr(b, records[i].vehicle, 32);
        putStr(b, records[i].corridor, 32);
        putPod(b, records[i].distanceKm);
        putPod(b, records[i].tonnage);
        putPod(b, records[i].etaHours);
    }
    writeAtomic(path, kLogMagic, b);
}

std::vector<TransitLogRecord> loadTransitLog(const std::string& path) {
    Bytes payload = readVerified(path, kLogMagic);
    Reader r(payload);
    std::uint32_t n = r.get<std::uint32_t>();
    std::vector<TransitLogRecord> out;
    for (std::uint32_t i = 0; i < n; ++i) {
        TransitLogRecord rec;
        rec.manifestId = r.str(24);
        rec.vehicle = r.str(32);
        rec.corridor = r.str(32);
        rec.distanceKm = r.get<double>();
        rec.tonnage = r.get<double>();
        rec.etaHours = r.get<double>();
        out.push_back(rec);
    }
    return out;
}

void saveManifest(const std::string& path, const Inventory::Manifest& m) {
    Bytes b;
    putStr(b, m.id(), 24);
    putPod(b, static_cast<std::uint32_t>(m.size()));
    for (std::size_t i = 0; i < m.size(); ++i) {
        const Inventory::ReliefCargo& c = m.at(i);
        putStr(b, c.name(), 32);
        putPod(b, static_cast<std::int32_t>(c.type()));
        putPod(b, static_cast<std::int32_t>(c.quantity()));
        putPod(b, c.unitWeightKg());
        putPod(b, static_cast<std::int32_t>(c.priority()));
    }
    writeAtomic(path, kManifestMagic, b);
}

Inventory::Manifest loadManifest(const std::string& path) {
    Bytes payload = readVerified(path, kManifestMagic);
    Reader r(payload);
    std::string id = r.str(24);
    std::uint32_t n = r.get<std::uint32_t>();
    Inventory::Manifest m(id, n ? n : 1);
    for (std::uint32_t i = 0; i < n; ++i) {
        std::string name = r.str(32);
        std::int32_t type = r.get<std::int32_t>();
        std::int32_t qty = r.get<std::int32_t>();
        double w = r.get<double>();
        std::int32_t prio = r.get<std::int32_t>();
        if (type < 0 || type >= Inventory::SUPPLY_TYPE_COUNT) throw PersistenceException("invalid supply type in file");
        m.add(Inventory::ReliefCargo(name, static_cast<Inventory::SupplyType>(type), qty, w, prio));
    }
    return m;
}

}  // namespace Persistence
}  // namespace ReliefOps
