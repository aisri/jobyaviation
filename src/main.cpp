#include <format>
#include <fstream>

#include "aircraft.h"
#include "charger.h"
#include "simulator.h"

#include <json/json.h>

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "FATAL! Expected Simulation Parameters file in JSON format" << endl;
        return -1;
    }

    ifstream jsonfs(argv[1]);

    Json::Value simconf;
    jsonfs >> simconf;

    // test driven development. ability to change test parameters using json DB
    // used JSON since, it is ubiquitous. can also have a csv file to lower typing
    Simulator::AirCraftsDB myDB { simconf["companies"] };

    Simulator::FlySpace sky_simulator(
        simconf["simulation_time"].asUInt(),
        simconf["evtols_count"].asUInt(),
        simconf["max_chargers"].asUInt(),
        myDB);

    sky_simulator.simulate();
    sky_simulator.print_stats();

    return 0;
}
