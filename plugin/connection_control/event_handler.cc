#include"event_handler.h"
#include<map>
#include<iostream>
#include"coordinator.h"
#include<string.h>
 Memory_store<std::string,uint64> data_store=Memory_store<std::string,uint64>();
Connection_control_coordinator coordinator=Connection_control_coordinator(10,100,1);
//std::map<std::string,uint32_t> map;
void Connection_event_handler::release_thd(MYSQL_THD THD){
          printf("%s \n","HelloWorld-ReleaseTHD");
}

void Connection_event_handler::receive_event(MYSQL_THD THD, unsigned int event_class,
                          const void *event)
{
  if (event_class == MYSQL_AUDIT_CONNECTION_CLASS)
  {
    struct mysql_event_connection *connection_event=
        (mysql_event_connection *) event;
    printf("host:%s,ip:%s,user:%s,connection id:%ld\n", connection_event->host,
           connection_event->ip, connection_event->user,
           connection_event->thread_id);
         data_store.add(std::string(connection_event->host),1);
         data_store.print();
        coordinator.coordinate(std::string(connection_event->user)+std::string(connection_event->host));

  }
  //如果达到阈值，则等待一段时间再返回
  //TODO 将失败次数保存到hash表中,然后IS从这个hash表中查询
  printf("%s \n", "Receive Event");
}