#include "nondet.h"

#include <unistd.h>
#include <time.h>

time_t time(time_t *tloc) {
	time_t retval;
	klee_make_symbolic(&retval, sizeof(retval), KLEE_MODEL("time"));

	/* any time (after the epoch) */
	klee_assume(retval >= 0);
	if (tloc)
		*tloc = retval;

	return retval;
}
