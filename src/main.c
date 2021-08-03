#include <stdio.h>
#include "client_api.h"
#include <string.h>
#include <signal.h>

int main(int argc, char const *argv[])
{
    net_client* client;
    client_err_t ret;
    char buf[128] = {0};
    int i =0;
    int j=20;

    signal(SIGPIPE,SIG_IGN);

    printf("---------------start test----------------------\n");

    client = create_client("test");
    if( !client)
    {
        printf("create_client failed---------------\n");
        return -1;
    }

    ret = connect_to_server(client, "127.0.0.1", 1234);
    if( ret )
    {
        printf("connect_to_server---------------\n");
        return -1;
    }
    printf("finish connect_to_server---------------ret = [%d]\n", ret);

    while(1)
    {
        snprintf(buf, 20, "Send test data %d", i);

        client_data_upload(client, buf, 20);
        i++;    
        sleep(1);
    }

    disconnect_from_server(client);

    while(1);
    
    return 0;
}
