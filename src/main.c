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
    mqtt_msg msg = {0};
    connect_opt opt;

    signal(SIGPIPE,SIG_IGN);
    
    opt.type = PROTOCAL_MQTT;
    opt.host = "127.0.0.1";
    opt.port = 1883;

    msg.mid = &j;
    msg.topic = "bbox/test";
    msg.payload = "Just a test";
    msg.payload_len = 11;
    msg.qos = 0;
    msg.retain = true;

    printf("---------------start test----------------------\n");

    fd = connect_to_server(&opt);
    if( fd == INVALID_HANDLE )
    {
        printf("create_client failed---------------\n");
        return -1;
    }

    printf("finish connect_to_server, handle[%d]\n", fd);
#if 0
    while(i < 10)
    {
        snprintf(buf, 20, "Send test data %d", i);

        client_data_upload(fd, buf, 20);
        i++;    
        sleep(1);
    }
#endif

while (1)
{
    client_data_upload(fd, &msg, 20);
    sleep(100);
}


    while(1);

    sleep(3);
    disconnect_from_server(fd);

    while(1);
    
    return 0;
}
