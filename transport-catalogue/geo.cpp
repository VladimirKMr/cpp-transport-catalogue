#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>

namespace geo {

    const int Earth_radius = 6371000;

    double ComputeDistance(Coordinates from, Coordinates to) {
        using namespace std;
        if (from == to) {
            return 0;
        }
        const double dr = M_PI / 180.0;
        return acos(sin(from.lat * dr) * sin(to.lat * dr)
            + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr))
            * Earth_radius;
    }

}  // namespace geo