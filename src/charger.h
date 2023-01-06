#include <semaphore>

using namespace std;

namespace Simulator {

class ChargingBay {
private:
    counting_semaphore<16> chargers;

public:
    ChargingBay(unsigned max_chargers)
        : chargers(max_chargers)
    {
    }

    void take_charger() {
        chargers.acquire();
    }

    void give_charger() {
        chargers.release();
    }
};
};