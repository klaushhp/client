#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

typedef enum 
{
    CLIENT_ERR_SUCCESS = 0,
    CLIENT_ERR_NOMEN = 1,
    CLIENT_ERR_INVALID_PARAM = 2,
    CLIENT_ERR_INVALID_OPERATION = 3,
    CLIENT_ERR_ERRNO = 4,
    CLIENT_ERR_SINGLETON = 5,
    CLIENT_ERR_CONN_LOST = 6,
} client_err_t;


#endif