#ifndef _AIRCRAFT_H_
#define _AIRCRAFT_H_

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

using namespace std;
using namespace std::chrono_literals;

#include "charger.h"

namespace Simulator {

struct AirCraftStats {
    string company;
    unsigned count;
    unsigned fly_time;
    unsigned charge_time;
    unsigned fault_count;
    unsigned total_distance;
    unsigned long passenger_miles;
    friend std::ostream& operator<<(std::ostream& stream, const AirCraftStats& stats);
};

std::ostream& operator<<(std::ostream& stream, const AirCraftStats& stats)
{
    return stream << std::setw(10) << stats.company
                  << std::setw(8) << stats.count
                  << std::fixed << std::setprecision(3)
                  << std::setw(15) << stats.fly_time
                  << std::setw(15) << stats.charge_time
                  << std::setw(10) << stats.total_distance
                  << std::setw(16) << stats.passenger_miles
                  << std::setw(8) << stats.fault_count << endl;
}

class AirCraftInfo {
public:
    AirCraftInfo(string builder, unsigned cruise_speed, unsigned battery_capacity, float charge_time, float energy_per_mile, unsigned num_passenger, float faults_per_hour)
        : company_(builder)
        , cruise_speed_(cruise_speed)
        , faults_per_hour_(faults_per_hour)
        , passenger_count_(num_passenger)
        , time_to_charge_(charge_time * 1000)
        , battery_capacity_(battery_capacity)
        , energy_use_at_cruise_(energy_per_mile)
        , fight_time_per_charge_(get_flight_time_per_charge())
    {
    }

    const string company_;
    const unsigned cruise_speed_;
    const float faults_per_hour_;
    const unsigned passenger_count_;
    const unsigned time_to_charge_;
    const unsigned battery_capacity_;
    const float energy_use_at_cruise_;
    const std::chrono::milliseconds fight_time_per_charge_;

private:
    const std::chrono::milliseconds get_flight_time_per_charge()
    {
        float miles_per_charge = battery_capacity_ / energy_use_at_cruise_;
        float time_per_charge = miles_per_charge / cruise_speed_;
        unsigned ints = static_cast<int>(time_per_charge * 1000 + 0.5);
        return std::chrono::milliseconds(ints);
    }

    friend std::ostream& operator<<(std::ostream& stream, const AirCraftInfo& craft);
};

std::ostream& operator<<(std::ostream& stream, const AirCraftInfo& craft)
{
    return stream << "\naircraft maker    : " << craft.company_
                  << "\ncruise speed      : " << craft.cruise_speed_
                  << "\nbattery capacity  : " << craft.battery_capacity_
                  << "\ntime to charge    : " << craft.time_to_charge_
                  << "\nenergy per mile   : " << craft.energy_use_at_cruise_
                  << "\npassenger count   : " << craft.passenger_count_
                  << "\nfaults per hour   : " << craft.faults_per_hour_
                  << "\nflight time/charge: " << craft.fight_time_per_charge_.count();
}

class AirCraft {
public:
    AirCraft(unsigned id, const AirCraftInfo& info, std::shared_ptr<ChargingBay> bay)
        : craft_info_(info)
        , evtol_id_(id)
    {
        // using fly-light design pattern to separate out const info from
        // changing parameters
        state_ = DOCKED;
        flying_time_ = 0;
        charging_time_ = 0;
        end_simulation_ = false;

        // co-owning charging bay object with other AirCraft's
        charging_bay_ = bay;
    }

    unsigned long get_passenger_miles() const
    {
        return craft_info_.passenger_count_ * flying_time_ * craft_info_.cruise_speed_;
    }

    unsigned get_faults() const
    {
        return std::ceil(craft_info_.faults_per_hour_ * flying_time_);
    }

    unsigned get_distance() const
    {
        return flying_time_ * craft_info_.cruise_speed_;
    }

    unsigned get_charging_time() const
    {
        return charging_time_;
    }

    unsigned get_flying_time() const
    {
        return flying_time_;
    }

    AirCraftStats get_stats() const
    {
        auto stats = AirCraftStats();
        stats.company = craft_info_.company_;
        stats.fault_count = get_faults();
        stats.fly_time = get_flying_time();
        stats.total_distance = get_distance();
        stats.charge_time = get_charging_time();
        stats.passenger_miles = get_passenger_miles();
        return stats;
    }

    void start_simulation()
    {
        while (!end_simulation_) {
            cout << "." << std::flush;
            switch (state_) {
            case DOCKED:
                state_ = CRUISING;
                break;
            case CRUISING:
                cruise_aircraft();
                state_ = CHARGING;
                break;
            case CHARGING:
                charge_aircraft();
                state_ = CRUISING;
                break;
            default:
                cout << "WARN: Invalid aircraft state" << endl;
                break;
            }
        }
        state_ = DOCKED;
    }

    void stop_simulation()
    {
        end_simulation_ = true;
        cv.notify_one();
    }

private:
    enum STATE {
        DOCKED,
        CHARGING,
        CRUISING
    };

    friend std::ostream& operator<<(std::ostream& stream, const STATE& st)
    {
        switch (st) {
        case DOCKED:
            return stream << "DOCKED";
        case CHARGING:
            return stream << "CHARGING";
        case CRUISING:
            return stream << "CRUISING";
        default:
            return stream << "UNKNOWN";
        }
    }

    STATE state_;
    bool end_simulation_;
    unsigned flying_time_;
    unsigned charging_time_;
    const AirCraftInfo& craft_info_;
    std::shared_ptr<ChargingBay> charging_bay_;

    const unsigned evtol_id_;
    std::mutex aircraft_lock;
    std::condition_variable cv;
    std::function<bool()> predicate = [&] { return end_simulation_; };

    void cruise_aircraft()
    {
        std::unique_lock<mutex> lock(aircraft_lock);

        auto start = chrono::system_clock::now();
        cv.wait_for(lock, craft_info_.fight_time_per_charge_, predicate);
        auto end = chrono::system_clock::now();

        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

        // track time spent flying - its possible simulation ends here,
        // so explicitly profile time taken after condition_variable returns
        flying_time_ += duration.count();
    }

    void charge_aircraft()
    {
        std::unique_lock<mutex> lock(aircraft_lock);

        auto start = chrono::system_clock::now();

        // get a charger
        charging_bay_->take_charger();

        // charge the aircraft
        auto charge_time_in_msec = chrono::milliseconds(craft_info_.time_to_charge_);
        cv.wait_for(lock, charge_time_in_msec, predicate);

        // release charger
        charging_bay_->give_charger();

        auto end = chrono::system_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);

        // total time spent for charging including wait
        charging_time_ += duration.count();
    }

    friend std::ostream& operator<<(std::ostream& stream, const AirCraft& craft);
};

std::ostream& operator<<(std::ostream& stream, const AirCraft& craft)
{
    return stream << "\nID                : " << craft.evtol_id_
                  << "\nSTATE             : " << craft.state_
                  << "\nCRUISE_TIME       : " << craft.flying_time_
                  << "\nCHARGE_TIME       : " << craft.charging_time_
                  << craft.craft_info_
                  << endl;
}

};

#endif