#ifndef _SIMULATOR_H_
#define _SIMULATOR_H_

#include <format>
#include <iomanip>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <json/json.h>

#include "aircraft.h"

namespace Simulator {

class AirCraftsDB {
public:
    AirCraftsDB(Json::Value evtol_companies)
    {
        for (const auto& craft : evtol_companies) {
            auto evtol = AirCraftInfo(
                craft["company"].asString(),
                craft["cruise-speed"].asUInt(),
                craft["battery-capacity"].asUInt(),
                craft["time-to-charge"].asFloat(),
                craft["energy-use-at-cruise"].asFloat(),
                craft["passenger-count"].asUInt(),
                craft["prob-faults-per-hour"].asFloat());

            evtol_crafts_.push_back(evtol);
        }
    }

    void print_database() const
    {
        for (const auto& craft : evtol_crafts_) {
            cout << craft << endl;
        }
    }

    const AirCraftInfo& get_aircraft_at_index(int index) const
    {
        int size = static_cast<int>(evtol_crafts_.size());
        if (index < size) {
            return evtol_crafts_[index];
        } else {
            return evtol_crafts_[0];
        }
    }

    unsigned get_aircraft_count() const
    {
        return evtol_crafts_.size();
    }

private:
    vector<const AirCraftInfo> evtol_crafts_;
};

class FlySpace {
public:
    FlySpace(float sim_time, unsigned evtol_count, unsigned num_chargers, const AirCraftsDB& db)
        : evtols_count_(evtol_count)
        , simulation_time_(sim_time)
        , db_(db)
        , bay_(new ChargingBay(num_chargers))
    {
    }

    void simulate()
    {
        // create aircraft objects
        srand(time(0));
        auto num_evtol_builders = db_.get_aircraft_count();
        for (unsigned i = 0; i < evtols_count_; i++) {
            auto index = rand() % num_evtol_builders;
            deployed_crafts_.emplace_back(
                new AirCraft(i, db_.get_aircraft_at_index(index), bay_));
            // cout << *deployed_crafts_[i] << endl; /* dump deployed aircraft */
        }

        cout << "*** Starting Simulation ***" << endl;
        // kick off aircraft thread's for simulation. detach them to
        // run independently w/o our intervention.
        for (const auto& evtol_craft : deployed_crafts_) {
            cout << ">>> Deploying AirCraft <<<";
            cout << *evtol_craft << endl;
            std::thread([&] { evtol_craft->start_simulation(); }).detach();
        }

        // lets sleep for simulation time to finish
        unsigned simulation_time = static_cast<unsigned>(simulation_time_ * 60.0);
        std::this_thread::sleep_for(std::chrono::seconds(simulation_time));

        // stop simulation
        cout << "*** Stopping Simulation ***" << endl;
        for (auto& dcraft : deployed_crafts_) {
            dcraft->stop_simulation();
        }

        // print simulation stats
        cout << "*** Simulation Report ***" << endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        print_stats();
    }

    void print_stats()
    {
        std::unordered_map<string, AirCraftStats> results;
        cout << ">>> per aircraft stats <<<" << endl;
        for (auto& dcraft : deployed_crafts_) {
            auto stats = dcraft->get_stats();
            cout << stats;
            auto it = results.find(stats.company);
            if (it == results.end()) {
                results[stats.company] = stats;
                results[stats.company].count = 1;
            } else {
                it->second.charge_time += stats.charge_time;
                it->second.fault_count += stats.fault_count;
                it->second.fly_time += stats.fly_time;
                it->second.passenger_miles += stats.passenger_miles;
                it->second.total_distance += stats.total_distance;
                it->second.count++;
            }
        }

        cout << "\n>>> ----- SUMMARY ----- <<<" << endl;
        cout << std::setw(10) << "COMPANY" << std::setw(8) << "#CRAFTS"
             << std::setw(15) << "FLIGHT-TIME:ms" << std::setw(15) << "CHARGE-TIME:ms"
             << std::setw(10) << "DISTANCE" << std::setw(16) << "PASSENGER-MILES"
             << std::setw(8) << "FAULTS" << endl;

        for (auto& [company, stats] : results) {
            unsigned cnt = stats.count;
            stats.fly_time /= (cnt * 1000);
            stats.charge_time /= (cnt * 1000);
            stats.total_distance /= (cnt * 1000);
            stats.passenger_miles *= cnt;
            stats.fault_count /= (cnt * 1000);
            cout << stats;
        }
    }

private:
    unsigned evtols_count_;
    float simulation_time_;

    const AirCraftsDB& db_;
    std::shared_ptr<ChargingBay> bay_;
    vector<unique_ptr<AirCraft>> deployed_crafts_;
};

};

#endif