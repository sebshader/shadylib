#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

#ifdef IRIX
#include <sys/endian.h>
#endif

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define HIOFFSET 1     
//# define LOWOFFSET 0                                                       
#else                                                                           
# define HIOFFSET 0    /* word offset to find MSB */                             
//# define LOWOFFSET 1    /* word offset to find LSB */                            
#endif

#define TRUE 1
#define FALSE 0

#define NORMHIPART 1094189056 //will this work on big-endian?

union tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};