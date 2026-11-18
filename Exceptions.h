// Exceptions.h - Unit IV: custom exception framework derived from std::exception
#pragma once
#include <exception>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace ReliefOps {

namespace detail {
inline std::string num(double v) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << v;
    return os.str();
}
}  // namespace detail

// Common base: every project exception IS-A std::exception.
class ReliefException : public std::exception {
public:
    explicit ReliefException(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override { return message_.c_str(); }
private:
    std::string message_;
};

class CargoOverweightException : public ReliefException {
public:
    CargoOverweightException(const std::string& vehicle, double loadKg, double capacityKg)
        : ReliefException("CargoOverweightException: " + vehicle + " cannot lift " +
                          detail::num(loadKg) + " kg (capacity " + detail::num(capacityKg) + " kg)"),
          excessKg_(loadKg - capacityKg) {}
    double excessKg() const { return excessKg_; }
private:
    double excessKg_;
};

class FuelDepletionException : public ReliefException {
public:
    FuelDepletionException(const std::string& vehicle, double neededKm, double availableKm)
        : ReliefException("FuelDepletionException: " + vehicle + " needs " + detail::num(neededKm) +
                          " km of fuel, only " + detail::num(availableKm) + " km left") {}
};

class DamagedCorridorException : public ReliefException {
public:
    explicit DamagedCorridorException(const std::string& corridor)
        : ReliefException("DamagedCorridorException: corridor '" + corridor + "' is impassable") {}
};

class StockShortageException : public ReliefException {
public:
    StockShortageException(const std::string& depot, const std::string& item, int requested, int available)
        : ReliefException("StockShortageException: " + depot + " has " + std::to_string(available) + " x " +
                          item + ", requested " + std::to_string(requested)) {}
};

class PersistenceException : public ReliefException {
public:
    explicit PersistenceException(const std::string& what)
        : ReliefException("PersistenceException: " + what) {}
};

}  // namespace ReliefOps
