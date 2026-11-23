// R-427: the buggy's and the drone's shapes in their own metres, shared by the ground that draws them (buggy.cpp, drone.cpp) and the ship
// that keeps them on their platforms in the maintenance room (game/vehicles_draw.cpp): the hull's stations (metres along the length from
// the centre) and their half widths and heights, the wheels, the drone's pods and skids
#pragma once

namespace BuggyShape {
constexpr double WHEEL_R = 0.42, WHEEL_X = 1.1, WHEEL_S = 0.85;   // wheel radius, longitudinal and lateral offsets of the hubs
constexpr double NOSE_F = 1.62, SHO_F = 1.0, CABF_F = 0.5, CABR_F = -0.8, TAIL_F = -1.55;
constexpr double NOSE_W = 0.42, SHO_W = 0.86, CAB_W = 0.9, TAIL_W = 0.72;
constexpr double FLOOR = 0.32, NOSE_B = 0.44, NOSE_T = 0.74, BELT = 0.92, ROOF = 1.5, TAIL_B = 0.4, TAIL_T = 1.02;
constexpr double CAM_F = 1.58, CAM_U = 0.84;   // where the nose camera sits (the pod spans 1.44..1.62 x 0.74..0.94)
}

// the wedge hull's stations as one row (the ship's models take either)
struct VehicleWedge {
    double NOSE_F, SHO_F, CABF_F, CABR_F, TAIL_F, NOSE_W, SHO_W, CAB_W, TAIL_W, FLOOR, NOSE_B, NOSE_T, BELT, ROOF, TAIL_B, TAIL_T, CAM_U;
};
constexpr VehicleWedge BUGGY_WEDGE = {BuggyShape::NOSE_F, BuggyShape::SHO_F, BuggyShape::CABF_F, BuggyShape::CABR_F, BuggyShape::TAIL_F, BuggyShape::NOSE_W,
                                      BuggyShape::SHO_W, BuggyShape::CAB_W, BuggyShape::TAIL_W, BuggyShape::FLOOR, BuggyShape::NOSE_B, BuggyShape::NOSE_T,
                                      BuggyShape::BELT, BuggyShape::ROOF, BuggyShape::TAIL_B, BuggyShape::TAIL_T, BuggyShape::CAM_U};

namespace DroneShape {   // the buggy's wedge, shorter
constexpr double NOSE_F = 1.25, SHO_F = 0.8, CABF_F = 0.4, CABR_F = -0.6, TAIL_F = -1.2;
constexpr double NOSE_W = 0.36, SHO_W = 0.7, CAB_W = 0.72, TAIL_W = 0.55;
constexpr double FLOOR = 0.35, NOSE_B = 0.45, NOSE_T = 0.72, BELT = 0.9, ROOF = 1.35, TAIL_B = 0.42, TAIL_T = 0.98;
constexpr double CAM_F = 1.21, CAM_U = 0.82;                         // the nose camera's lens (the pod spans 1.07..1.25 x 0.72..0.92)
constexpr double POD_F = 1.35, POD_S = 1.45, POD_U = 1.05, POD_R = 0.5;   // the thrust pods: fore and aft, sideways, their height and radius
constexpr double SKID_S = 0.5;                                       // the skids' half track
}
constexpr VehicleWedge DRONE_WEDGE = {DroneShape::NOSE_F, DroneShape::SHO_F, DroneShape::CABF_F, DroneShape::CABR_F, DroneShape::TAIL_F, DroneShape::NOSE_W,
                                      DroneShape::SHO_W, DroneShape::CAB_W, DroneShape::TAIL_W, DroneShape::FLOOR, DroneShape::NOSE_B, DroneShape::NOSE_T,
                                      DroneShape::BELT, DroneShape::ROOF, DroneShape::TAIL_B, DroneShape::TAIL_T, DroneShape::CAM_U};
