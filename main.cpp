#include <unistd.h>
#include "server/webserver.h"

int main()
{
    WebServer server(
        8888, 3, 20000, false,                      //端口，ET模式，timeout，优雅退出
        3306, "root", "root", "webserver",          //Mysql配置
        12, 6, true, 1, 1024);                      //连接池数量，线程池数量，日至开关，日志等级，日至异步队列容量

    server.Start();
    return 0;
}
