// 28 april 2019
#include <errno.h>
#include <inttypes.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "testing.h"

static jmp_buf timeout_ret;

static void onTimeout(int sig)
{
	longjmp(timeout_ret, 1);
}

void testingRunWithTimeout(testingT *t, int64_t timeout, void (*f)(testingT *t, void *data), void *data, const char *comment, int failNowOnError)
{
	char *timeoutstr;
	sig_t prevsig;
	struct itimerval timer, prevtimer;
	int setitimerError = 0;

	timeoutstr = testingTimerNsecString(timeout);
	prevsig = signal(SIGALRM, onTimeout);

	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = timeout / testingTimerNsecPerSec;
	timer.it_value.tv_usec = (timeout % testingTimerNsecPerSec) / testingTimerNsecPerUsec;
	if (setitimer(ITIMER_REAL, &timer, &prevtimer) != 0) {
		setitimerError = errno;
		testingTErrorf(t, "error applying %s timeout: %s", comment, strerror(setitimerError));
		goto out;
	}

	if (setjmp(timeout_ret) == 0) {
		(*f)(t, data);
		failNowOnError = 0;		// we succeeded
	} else
		testingTErrorf(t, "%s timeout passed (%s)", comment, timeoutstr);

out:
	if (setitimerError == 0)
		setitimer(ITIMER_REAL, &prevtimer, NULL);
	signal(SIGALRM, prevsig);
	testingFreeTimerNsecString(timeoutstr);
	if (failNowOnError)
		testingTFailNow(t);
}
