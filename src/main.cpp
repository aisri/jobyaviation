/**
 * @file main.cpp
 * @author Sreekanth, Boga (aisri@github.com)
 * @brief entry point for program
 *
 */

#include <fstream>
#include <iostream>
#include <json/json.h>

#include "simulator.h"

using namespace std;
using namespace Simulator;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "FATAL! Expected Simulation Parameters file in JSON format" << endl;
        return -1;
    }

    std::ifstream jsonfs(argv[1], std::ifstream::binary);
    Json::Value simconf;
    jsonfs >> simconf;

    // test driven development. ability to change test parameters using json DB
    // used JSON since, it is ubiquitous. can also have a csv file to lower typing
    AirCraftsDB myDB (simconf["companies"]);
    // myDB.print_database();

    Simulator::FlySpace sky_simulator(
        simconf["simulation_time"].asFloat(),
        simconf["evtols_count"].asUInt(),
        simconf["max_chargers"].asUInt(),
        myDB);

    sky_simulator.simulate();

    return 0;
}
