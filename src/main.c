#include <stdio.h>
#include "client_api.h"
#include <string.h>
#include <signal.h>

int main(int argc, char const *argv[])
{
    client_err_t ret;
    client_t fd;
    char buf[128] = {0};
    int i =0;
    int j=20;

    signal(SIGPIPE,SIG_IGN);

    char *p = "127.0.0.1";
    connect_opt opt;

    opt.type = PROTOCAL_TCP;
    opt.host = p;
    opt.port = 1234;
    opt.tls_enable = false;

    printf("---------------start test----------------------\n");

    fd = connect_to_server(&opt);
    if( fd == INVALID_HANDLE )
    {
        printf("create_client failed---------------\n");
        return -1;
    }

    printf("finish connect_to_server---------------ret\n");
#if 0
    while(i < 10)
    {
        snprintf(buf, 20, "Send test data %d", i);

        client_data_upload(fd, buf, 20);
        i++;    
        sleep(1);
    }
#endif

    while(1);

    sleep(3);
    disconnect_from_server(fd);

    while(1);
    
    return 0;
}
