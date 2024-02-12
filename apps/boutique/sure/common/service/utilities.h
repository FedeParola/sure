/*
 * Some sort of Copyright
 */

#ifndef __UTILITIES__
#define __UTILITIES__

#include "message.h"

#define NANOSMOD 1000000000

static void MoneySum(Money *total, Money *add)
{
	total->Units = total->Units + add->Units;
	total->Nanos = total->Nanos + add->Nanos;

	if ((total->Units == 0 && total->Nanos == 0)
	    || (total->Units > 0 && total->Nanos >= 0)
	    || (total->Units < 0 && total->Nanos <= 0)) {
		// same sign <units, nanos>
		total->Units += (int64_t)(total->Nanos / NANOSMOD);
		total->Nanos = total->Nanos % NANOSMOD;
	} else {
		// different sign. nanos guaranteed to not to go over the limit
		if (total->Units > 0) {
			total->Units--;
			total->Nanos += NANOSMOD;
		} else {
			total->Units++;
			total->Nanos -= NANOSMOD;
		}
	}
}

static void MoneyMultiplySlow(Money *total, uint32_t n)
{
	for (; n > 1 ;) {
		MoneySum(total, total);
		n--;
	}
}

#endif /* __UTILITIES__ */