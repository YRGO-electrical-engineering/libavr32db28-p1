/**
 * @file AVR32DB28 joystick driver.
 */
#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of joystick directions.
 */
typedef enum
{
    JOYSTICK_CENTER, ///< Joystick is at center.
    JOYSTICK_UP,     ///< Joystick is pointing up.
    JOYSTICK_DOWN,   ///< Joystick is pointing down.
    JOYSTICK_LEFT,   ///< Joystick is pointing left.
    JOYSTICK_RIGHT,  ///< Joystick is pointing right.
} joystick_dir_t;

/**
 * @brief Initialize joystick.
 */
void joystick_init(void);

/**
 * @brief Get joystick state.
 *
 * @return Joystick state in terms of direction.
 */
joystick_dir_t joystick_read(void);

/**
 * @brief Check if the joystick is pressed.
 *
 * @return True if pressed, false if not.
 */
bool joystick_pressed(void);

#endif /** JOYSTICK_H_ */
