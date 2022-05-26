#ifndef __PAGE_LOG_H__
#define __PAGE_LOG_H__

#define _DEBUG 1

#define debug_cond(cond, fmt, args...) \
    do {                               \
        if (cond)                      \
            printf(fmt, ##args);       \
    } while (0)

#define debug(fmt, args...) \
    debug_cond(_DEBUG, fmt, ##args)

#endif
