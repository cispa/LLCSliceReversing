#ifndef UTILS_H
#define UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/perf_event.h>
#include <sched.h>

/**
 * Pins the pid to the given core
 *
 * :param pid: The pid to pin
 * :param core: The core where pid should be pinned to
 */
void utils_pin_to_core(pid_t pid, int core);

size_t utils_get_physical_address(size_t vaddr);

int utils_event_open(enum perf_type_id type, __u64 config, __u64 exclude_kernel,
                     __u64 exclude_hv, __u64 exclude_callchain_kernel, int cpu);

int utils_find_index_of_nth_largest_size_t(size_t *list, size_t nmemb,
                                           size_t skip);
#ifdef __cplusplus
}
#endif

#endif // UTILS_H
