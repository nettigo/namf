
#if defined(INTL_EN)
#include "lang/intl_en.h"
#define NTP_SERVER "0.europe.pool.ntp.org"
#elif defined(INTL_PL)
#include "lang/intl_pl.h"
#define NTP_SERVER "0.pl.pool.ntp.org"
#elif defined(INTL_RO)
#include "lang/intl_ro.h"
#define NTP_SERVER "0.ro.pool.ntp.org"
#elif defined(INTL_HU)
#include "lang/intl_hu.h"
#define NTP_SERVER "0.hu.pool.ntp.org"
#else
#include "lang/intl_en.h"
#define NTP_SERVER "0.europe.pool.ntp.org"
#endif
