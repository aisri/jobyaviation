#include <format>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <json/json.h>

#include "aircraft.h"

namespace Simulator {
class FlySpace {
public:
    FlySpace(unsigned sim_time, unsigned evtol_count, unsigned num_chargers, const AirCraftsDB& db)
        : evtols_count_(evtol_count)
        , simulation_time_(sim_time)
        , db_(db)
        , bay_(new ChargingBay(num_chargers))
    {
    }

    void simulate()
    {
        srand(time(0));

        for (int i = 0; i < evtols_count_; i++) {
            auto index = rand() % db_.get_aircraft_count();
            auto craft_info = db_.get_aircraft_at_index(index);

            deployed_crafts_.push_back(AirCraft(
                static_cast<unsigned>(deployed_crafts_.size()),
                craft_info, bay_));

            // kick off aircraft thread's for simulation. detach them to
            // run independently w/o our intervention.
            std::thread([&] { deployed_crafts_.back().start_simulation(); }).detach();
        }

        // lets sleep for simulation time to finish and
        std::this_thread::sleep_for(std::chrono::minutes(simulation_time_));

        for (auto& dcraft : deployed_crafts_) {
            dcraft.stop_simulation();
        }
    }

    void print_stats()
    {
        std::unordered_map<string, AirCraftStats> results;
        for (auto& dcraft : deployed_crafts_) {
            auto stats = dcraft.get_stats();
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
        auto format = [](string bd, unsigned num, float ft, float ct, float d, float pm, float fl) {
            cout << std::setw(10) << bd << std::setw(8) << num << std::setw(12) << ft
                 << std::setw(12) << ct << std::setw(10) << d << std::setw(16) << pm
                 << std::setw(8) << fl << endl;
        };

        string fmt = "{:10}{:8}{:12}{:12}{:10}{:16}{:8}";
        cout << "COMPANY", "#EVTOLS", "FLIGHT-TIME", "CHARGE-TIME", "DISTANCE",
            "PASSENGER-MILES", "FAULTS";
        for (const auto& [company, stats] : results) {
            unsigned cnt = stats.count;
            format(company, cnt,
                stats.fly_time / cnt,
                stats.charge_time / cnt,
                stats.total_distance / cnt,
                stats.passenger_miles * cnt,
                stats.fault_count / cnt);
        }
    }

private:
    unsigned evtols_count_;
    unsigned simulation_time_;

    const AirCraftsDB& db_;
    std::shared_ptr<ChargingBay> bay_;
    vector<AirCraft> deployed_crafts_;
};


class AirCraftsDB {
public:
    AirCraftsDB(Json::Value evtol_companies)
    {
        for (const auto& craft : evtol_companies) {
            auto shared = std::make_shared<AirCraftInfo>(new AirCraftInfo(
                craft["company"].asString(),
                craft["cruise-speed"].asUInt(),
                craft["battery-capacity"].asUInt(),
                craft["time-to-charge"].asFloat(),
                craft["energy-use-at-cruise"].asFloat(),
                craft["passenger-count"].asUInt(),
                craft["prob-faults-per-hour"].asFloat()));

            evtol_crafts_.push_back(shared);
        }
    }

    void print_database() const
    {
        for (const auto& craft : evtol_crafts_) {
            cout << craft << endl;
        }
    }

    std::shared_ptr<AirCraftInfo> get_aircraft_at_index(int index) const
    {
        if (index < evtol_crafts_.size()) {
            return evtol_crafts_[index];
        }
        return make_shared<AirCraftInfo>();
    }

    unsigned get_aircraft_count() const
    {
        return evtol_crafts_.size();
    }

private:
    vector<shared_ptr<AirCraftInfo>> evtol_crafts_;
};

};