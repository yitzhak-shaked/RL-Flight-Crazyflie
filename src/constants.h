#ifndef LEARNING_TO_FLY_CONSTANTS_H
#define LEARNING_TO_FLY_CONSTANTS_H

#include <array>
#include <cstddef>

namespace learning_to_fly {
    namespace constants {
        // Target position for position-to-position training
        template<typename T>
        constexpr T TARGET_POSITION_X = T(0.0);
        
        template<typename T>
        constexpr T TARGET_POSITION_Y = T(1.2);
        
        template<typename T>
        constexpr T TARGET_POSITION_Z = T(0.0);
        
        // Helper function to get target position as array for array initialization
        template<typename T>
        constexpr std::array<T, 3> get_target_position_array() {
            return {TARGET_POSITION_X<T>, TARGET_POSITION_Y<T>, TARGET_POSITION_Z<T>};
        }
        
        // Helper function to get target position by filling an array
        template<typename T>
        constexpr void get_target_position(T target_pos[3]) {
            target_pos[0] = TARGET_POSITION_X<T>;
            target_pos[1] = TARGET_POSITION_Y<T>;
            target_pos[2] = TARGET_POSITION_Z<T>;
        }
        
        // Cylindrical obstacles (pipes) configuration
        struct CylindricalObstacle {
            double x;        // X position
            double y;        // Y position
            double radius;   // Radius in meters
            double z_min;    // Minimum Z (bottom of pipe)
            double z_max;    // Maximum Z (top of pipe)
        };
        
        // Array of cylindrical obstacles - can be extended in the future
        constexpr CylindricalObstacle OBSTACLES[] = {
            // {0.0, 1.0, 0.2, -1.0, 1.0}  // x=0m, y=1m, radius=20cm, z from -1m to +1m
        };
        
        constexpr size_t NUM_OBSTACLES = sizeof(OBSTACLES) / sizeof(OBSTACLES[0]);
        
        // Planar obstacles (walls) configuration
        // Plane is defined by: point on plane, normal vector, and bounds
        struct PlanarObstacle {
            // Point on the plane
            double point_x;
            double point_y;
            double point_z;
            
            // Normal vector (should be normalized)
            double normal_x;
            double normal_y;
            double normal_z;
            
            // Half-thickness of the wall (collision if distance < thickness)
            double thickness;
            
            // Bounding box to limit the plane extent (min/max in each dimension)
            double x_min;
            double x_max;
            double y_min;
            double y_max;
            double z_min;
            double z_max;
        };
        
        // Array of planar obstacles - can be extended in the future
        constexpr PlanarObstacle PLANAR_OBSTACLES[] = {
            // // Vertical wall perpendicular to Y-axis at y=1.0m (halfway between origin and target)
            // {1.0, 0.6, 0.0,    // Point on plane (x, y, z)
            //  0.0, 1.0, 0.0,    // Normal vector (pointing in +Y direction)
            //  0.1,              // Thickness (10cm)
            //  0.0, 1.0,         // X bounds (from 0m to +1m, 1m wide wall)
            //  0.95, 1.05,       // Y bounds (wall thickness region around y=1.0)
            //  -1.0, 1.0}        // Z bounds (from -1m to +1m height, 2m tall wall)
        };
        
        constexpr size_t NUM_PLANAR_OBSTACLES = sizeof(PLANAR_OBSTACLES) / sizeof(PLANAR_OBSTACLES[0]);
    }
}

#endif
