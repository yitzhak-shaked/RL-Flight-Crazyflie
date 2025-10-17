#ifndef LEARNING_TO_FLY_CONSTANTS_H
#define LEARNING_TO_FLY_CONSTANTS_H

#include <array>

namespace learning_to_fly {
    namespace constants {
        // Target position for position-to-position training
        template<typename T>
        constexpr T TARGET_POSITION_X = T(0.0);
        
        template<typename T>
        constexpr T TARGET_POSITION_Y = T(0.4);
        
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
    }
}

#endif
