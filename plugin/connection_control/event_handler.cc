#include "event_handler.h"
#include <map>
#include <iostream>
#include <string.h>
#include "coordinator.h"
/* Variables definition */
extern Memory_store<std::string, int64>* data_store ;
extern connection_control::Connection_control_coordinator* coordinator;

void Connection_event_handler::release_thd(MYSQL_THD THD)
{
  DBUG_PRINT("info",("Release connection"));
}

void Connection_event_handler::receive_event(MYSQL_THD THD,
                                             unsigned int event_class,
                                             const void *event)
{
  DBUG_ENTER("Connection_event_handler::receive_event");
  //if threshold is  no zero value ,then go down
  if(coordinator->getGVariables().getFailedConnectionsThreshold()<=0){
     DBUG_VOID_RETURN;
  }
  if (event_class == MYSQL_AUDIT_CONNECTION_CLASS)
  {
    struct mysql_event_connection *connection_event=
        (mysql_event_connection *) event;
    // Only record for connection ,not disconnection
    if (connection_event->event_subclass == MYSQL_AUDIT_CONNECTION_CONNECT)
    {
      std::string connection_key= std::string("");
      //'user'@'host'
      connection_key.append("'")
          .append(connection_event->user)
          .append("'")
          .append("@")
          .append("'")
          .append(connection_event->host)
          .append("'");
      DBUG_PRINT("info", ("%s failed login", connection_key));
      if (connection_event->status)
      {
        /**
         * @brief Cache Failure count
         *
         */
        int64 current_count= 0;
        DBUG_PRINT("info",("Login failure, start get data lock"));
        coordinator->write_lock();
        DBUG_PRINT("info",("Success get data store lock"));
        if (data_store->contains(connection_key))
        {
          int64 present_count= data_store->find(connection_key);
          data_store->put(connection_key, present_count + 1);
          current_count= present_count + 1;
        }
        else
        {
          data_store->add(connection_key, 1);
          current_count= 1;
        }

        coordinator->unlock();
        coordinator->coordinate(current_count, THD);
      }
      else
      {
        // Get write lock
        DBUG_PRINT("info",("Login success, start get data store write lock"));
        //According mysql work log FR2.1 , a successful login will keep delay  once delay is triggered
        if (data_store->contains(connection_key))
        {
           coordinator->coordinate( data_store->find(connection_key), THD);
        }       
        coordinator->write_lock();
        DBUG_PRINT("info",("Success data store write lock"));          
        data_store->erase(connection_key);
        coordinator->unlock();
      }
    }
  }

  DBUG_VOID_RETURN;
}