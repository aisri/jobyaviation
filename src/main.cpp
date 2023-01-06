#include <fstream>
#include <iostream>
#include <vector>

#include <json/json.h>

#include "aircraft.h"
#include "charger.h"

namespace Simulator {
class FlySpace {
public:
    FlySpace(unsigned sim_time, unsigned evtol_count, unsigned num_chargers, const AircraftsDB& db)
        : evtols_count_(evtol_count)
        , simulation_time_(sim_time)
        , db_(db)
        , bay_(new ChargingBay(num_chargers))
    {
    }

    void simulate()
    {
    }

    void print_stats()
    {
    }

private:
    unsigned evtols_count_;
    unsigned simulation_time_;
    const AircraftsDB& db_;
    std::shared_ptr<ChargingBay> bay_;
};

class AircraftsDB {
public:
    AircraftsDB(Json::Value evtol_db)
    {
    }

private:
    vector<AirCraftInfo> evtol_crafts;
};
};

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "FATAL! Expected Simulation Parameters file in JSON format" << endl;
        return -1;
    }

    ifstream jsonfs(argv[1]);

    Json::Value simconf;
    jsonfs >> simconf;

    Simulator::AircraftsDB myDB { simconf["companies"] };

    Simulator::FlySpace sky_simulator(
        simconf["simulation_time"].asUInt(),
        simconf["evtols_count"].asUInt(),
        simconf["max_chargers"].asUInt(),
        myDB);

    sky_simulator.simulate();
    sky_simulator.print_stats();

    return 0;
}