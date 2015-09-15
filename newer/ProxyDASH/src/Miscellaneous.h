/*
 * Miscellaneous.h
 *
 *  Created on: 27/apr/2015
 *      Author: angelo
 */

#ifndef MISCELLANEOUS_H_
#define MISCELLANEOUS_H_


#define TIME_STAT_UPDATE		10
#define BLOCK_SIZE_STATS_BYTE	250000
#define START_FRAGMENT 5


#define MAX_DEBUG_LEVEL		3
#define MEDIUM_DEBUG_LEVEL	2
#define MIN_DEBUG_LEVEL		1
#define NO_DEBUG_LEVEL		0

#ifndef DEBUG_LEVEL
//#define DEBUG_LEVEL NO_DEBUG_LEVEL
//#define DEBUG_LEVEL MIN_DEBUG_LEVEL
//#define DEBUG_LEVEL MEDIUM_DEBUG_LEVEL
#define DEBUG_LEVEL MAX_DEBUG_LEVEL
#endif


#if DEBUG_LEVEL==NO_DEBUG_LEVEL

#define debug_low(...)
#define debug_medium(...)
#define debug_high(...)

#elif  DEBUG_LEVEL==MIN_DEBUG_LEVEL

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...)
#define debug_high(...)

#elif  DEBUG_LEVEL==MEDIUM_DEBUG_LEVEL

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...) printf (__VA_ARGS__);
#define debug_high(...)

#else

#define debug_low(...) printf (__VA_ARGS__);
#define debug_medium(...) printf (__VA_ARGS__);
#define debug_high(...) printf (__VA_ARGS__);

#endif



#endif /* MISCELLANEOUS_H_ */
