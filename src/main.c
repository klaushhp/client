#include <stdio.h>
#include "client_api.h"

int main(int argc, char const *argv[])
{
    client_err_t ret;

    printf("---------------start test----------------------\n");

    ret = create_client("test");
    if( ret )
    {
        printf("create_client failed---------------\n");
        return -1;
    }

    ret = connect_to_server("127.0.0.1", 1234);
    if( ret )
    {
        printf("connect_to_server---------------\n");
        return -1;
    }
    printf("finish connect_to_server---------------ret = [%d]\n", ret);

    while(1);
    
    return 0;
}
