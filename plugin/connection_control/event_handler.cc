#include "event_handler.h"
#include <map>
#include <iostream>
#include <string.h>



Memory_store<std::string, int64> data_store;
connection_control::Connection_control_coordinator coordinator=
    connection_control::Connection_control_coordinator(
        connection_control::DEFAULT_THRESHOLD,
        connection_control::DEFAULT_MIN_DELAY,
        connection_control::DEFAULT_MAX_DELAY);

void Connection_event_handler::release_thd(MYSQL_THD THD)
{
  printf("%s \n", "HelloWorld-ReleaseTHD");
}

void Connection_event_handler::receive_event(MYSQL_THD THD,
                                             unsigned int event_class,
                                             const void *event)
{
  if (event_class == MYSQL_AUDIT_CONNECTION_CLASS)
  {
    struct mysql_event_connection *connection_event=
        (mysql_event_connection *) event;
    // Only record for connection ,not disconnection
    if (connection_event->event_subclass == MYSQL_AUDIT_CONNECTION_CONNECT)
    {
       std::string connection_key= std::string(connection_event->user) + "%" +
                                  std::string(connection_event->host);
      if (connection_event->status)
      {
        /**
         * @brief Cache Failure count
         *
         */
        printf("host:%s,ip:%s,user:%s,connection id:%ld\n",
               connection_event->host, connection_event->ip,
               connection_event->user, connection_event->thread_id);
        int64 current_count= 0;
        coordinator.write_lock();
        if (data_store.contains(connection_key))
        {
          int64 present_count= data_store.find(connection_key);
          data_store.put(connection_key, present_count + 1);
          current_count= present_count + 1;
        }
        else
        {
          data_store.add(connection_key, 1);
          current_count= 1;
        }

        data_store.print();
        coordinator.unlock();
        coordinator.coordinate(current_count, THD);
      }
      else
      {
    //    coordinator.write_lock();
        data_store.erase(connection_key);
 //       coordinator.unlock();
      }
    }
  }
  printf("%s \n", "Receive Event");
}