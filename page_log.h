#ifndef __PAGE_LOG_H__
#define __PAGE_LOG_H__

#define LOG_ENABLE 1

#if LOG_ENABLE
#include <stdio.h>
#define p_log(fmt, args...) printf("[Log_PM] " fmt "\n", ##args)
#define p_warning(fmt, args...) printf("[Warning_PM] " fmt "\n", ##args)
#define p_error(fmt, args...) printf("[Error_PM] " fmt "\n", ##args)
#else
#define p_log(...)
#define p_warning(...)
#define p_error(...)
#endif

#endif /* __PAGE_LOG_H__ */
