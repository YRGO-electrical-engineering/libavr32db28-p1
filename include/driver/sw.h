/**
 * @file AVR32DB28 switch driver.
 */
#ifndef SW_H_
#define SW_H_

#include <stdbool.h>

/**
 * @brief Enumeration of switch IDs.
 */
typedef enum
{
    SW1, ///< Switch 1.
    SW2, ///< Switch 2.
    SW3, ///< Switch 3.
} switch_id_t;

/**
 * @brief Initialize all switches.
 */
void sw_init(void);

/**
 * @brief Read switch state.
 *
 * @param[in] sw Switch ID.
 *
 * @return True if the switch is pressed, false otherwise.
 */
bool sw_read(switch_id_t sw);

#endif /** SW_H_ */
