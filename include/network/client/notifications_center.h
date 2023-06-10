#ifndef NOTIFICATIONS_CENTER_H
#define NOTIFICATIONS_CENTER_H


/* -------------------------------- INCLUDE --------------------------------- */

#include "network/network_macros.h"

/* -------------------------------- FUNCTIONS ------------------------------- */

uint8_t notifications_center_init(void);
void *notifications_center_loop(__attribute__((unused)) void *arg);

/* -------------------------------------------------------------------------- */

#endif /* NOTIFICATIONS_CENTER_H */
