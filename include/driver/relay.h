/**
 * @file AVR32DB28 relay driver.
 */
#ifndef RELAY_H_
#define RELAY_H_

#include <stdbool.h>

/**
 * @brief Initialize relay.
 */
void relay_init(void);

/**
 * @brief Get relay state.
 *
 * @return True if the relay is closed, false if it is open.
 */
bool relay_read(void);

/**
 * @brief Set relay state.
 *
 * @param[in] state Relay state (true = on, false = off).
 */
void relay_write(bool state);

/**
 * @brief Toggle relay state.
 */
void relay_toggle(void);

#endif /** RELAY_H_ */
