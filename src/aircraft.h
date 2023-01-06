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
class AirCraftInfo {
public:
    const unsigned time_to_charge_;
    const std::chrono::milliseconds fight_time_per_charge_;

    AirCraftInfo(string builder, unsigned cruise_speed, unsigned battery_capacity, unsigned charge_time, unsigned energy_use_at_cruise, unsigned num_passenger, float faults_per_hour)
        : company_(builder)
        , cruise_speed_(cruise_speed)
        , battery_capacity_(battery_capacity)
        , time_to_charge_(charge_time)
        , energy_use_at_cruise_(energy_use_at_cruise)
        , passenger_count_(num_passenger)
        , faults_per_hour_(faults_per_hour)
        , fight_time_per_charge_(get_flight_time_per_charge())
    {
    }

private:
    const string company_;
    const unsigned cruise_speed_;
    const unsigned battery_capacity_;
    const unsigned energy_use_at_cruise_;
    const unsigned passenger_count_;
    const float faults_per_hour_;

    const std::chrono::milliseconds get_flight_time_per_charge()
    {
        float miles_per_charge = battery_capacity_ / energy_use_at_cruise_;
        float time_per_charge = miles_per_charge / cruise_speed_;
        unsigned ints = static_cast<int>(time_per_charge * 1000 + 0.5);
        return std::chrono::milliseconds(ints);
    }

    friend std::ostream& operator<<(std::ostream& stream, const AirCraft& craft);
};

class AirCraft {
public:
    AirCraft(unsigned id, AirCraftInfo& info, std::shared_ptr<ChargingBay> bay)
        : evtol_id_(id)
        , craft_info_(info)
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

    unsigned long get_passenger_miles()
    {
        return passenger_count_ * flying_time_ * cruise_speed_;
    }

    unsigned get_faults()
    {
        return std::ceil(faults_per_hour_ * flying_time_);
    }

    unsigned get_distance()
    {
        return flying_time_ * cruise_speed_;
    }

    unsigned get_charging_time()
    {
        return charging_time_;
    }

    void start_simulation()
    {
        while (!end_simulation_) {
            cout << evtol_id_ << ": " << state_ << " --> ";
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
            cout << state_ << endl;
        }
        cout << evtol_id_ << ": " << state_ << " --> DOCKED";
        state_ = DOCKED;
    }

    void stop_simulation()
    {
        end_simulation_ = true;
        cv.notify_all();
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
    std::shared_ptr<ChargingBay> charging_bay_;

    AirCraftInfo& craft_info_;
    const unsigned evtol_id_;
    std::condition_variable cv;
    std::unique_lock<mutex> lock;
    std::function<bool()> predicate = [&] { return end_simulation_; };

    bool cruise_aircraft()
    {
        auto start = chrono::system_clock::now();
        cv.wait_for(lock, craft_info_.fight_time_per_charge_, predicate);
        auto duration = chrono::system_clock::now() - start;

        // track time spent flying - its possible simulation ends
        flying_time_ += duration.count();
        return !predicate();
    }

    bool charge_aircraft()
    {
        auto start = chrono::system_clock::now();

        // get a charger
        charging_bay_->take_charger();

        // charge the aircraft
        auto charge_time_in_msec = chrono::milliseconds(craft_info_.time_to_charge_ * 1000);
        cv.wait_for(lock, charge_time_in_msec, predicate);

        // release charger
        charging_bay_->give_charger();

        auto duration = chrono::system_clock::now() - start;

        // total time spent for charging including wait
        charging_time_ += duration.count();

        return !predicate();
    }

    friend std::ostream& operator<<(std::ostream& stream, const AirCraft& craft);
};

std::ostream& operator<<(std::ostream& stream, const AirCraft& craft)
{
    return stream << "ID: " << craft.evtol_id_
                  << "\nMAKE        : " << craft.craft_info_.company_
                  << "\nSTATE       : " << craft.state_
                  << "\nCRUISE_TIME : " << craft.flying_time_
                  << "\nCHARGE_TIME : " << craft.charging_time_
#if 1
                  << "\ncruise speed      : " << craft.craft_info_.cruise_speed_
                  << "\nbattery capacity  : " << craft.craft_info_.battery_capacity_
                  << "\ntime to charge    : " << craft.craft_info_.time_to_charge_
                  << "\nenergy per mile   : " << craft.craft_info_.energy_use_at_cruise_
                  << "\npassenger count   : " << craft.craft_info_.passenger_count_
                  << "\nfaults per hour   : " << craft.craft_info_.faults_per_hour_
                  << "\nflight time/charge: " << craft.craft_info_.fight_time_per_charge_.count()
#endif
                  << endl;
};
};
