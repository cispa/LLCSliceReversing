#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sched.h>

/**
 * Pins the pid to the given core
 *
 * :param pid: The pid to pin
 * :param core: The core where pid should be pinned to
 */
void utils_pin_to_core(pid_t pid, int core);

size_t utils_get_physical_address(size_t vaddr);

#ifdef __cplusplus
}
#endif

#endif // UTILS_H
