#include "event_handler.h"
#include <map>
#include <iostream>
#include <string.h>
#include "coordinator.h"
#include "sql_class.h"

/* Variables definition */
extern Memory_store<std::string, int64>* data_store ;
extern connection_control::Connection_control_coordinator* coordinator;
void make_hash_key(MYSQL_THD thd, std::string &s);
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
      make_hash_key(THD, connection_key);
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
        printf("faile login connection key %s\n",connection_key.c_str());
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
        printf("connection key %s\n",connection_key.c_str());
        if (data_store->contains(connection_key))
        {
           coordinator->coordinate( data_store->find(connection_key)+1, THD);
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
/**
  Create hash key of the format '<user>'@'<host>'.
  Policy:
  1. Use proxy_user information if available. Else if,
  2. Use priv_user/priv_host if either of them is not empty. Else,
  3. Use user/host

  @param [in] thd        THD pointer for getting security context
  @param [out] s         Hash key is stored here
*/

void make_hash_key(MYSQL_THD thd, std::string &s) {
  /* Our key for hash will be of format : '<user>'@'<host>' */

  /* If proxy_user is set then use it directly for lookup */
   Security_context *sctx= thd->security_ctx;
  const char *proxy_user = sctx->proxy_user;
  if (proxy_user && *proxy_user) {
    s.append(proxy_user);
  } /* else if priv_user and/or priv_host is set, then use them */
  else {
    const char *priv_user = sctx->priv_user;
    const char *priv_host = sctx->priv_host;
    if (*priv_user || *priv_host) {
      s.append("'");

      if (*priv_user) s.append(priv_user);

      s.append("'@'");

      if (*priv_host) s.append(priv_host);

      s.append("'");
    } else {
      const char *user = sctx->user;
      const char *host = sctx->host;
      const char *ip = sctx->ip;

      s.append("'");

      if (user && *user) s.append(user);

      s.append("'@'");

      if (host && *host)
        s.append(host);
      else if (ip && *ip)
        s.append(ip);

      s.append("'");
    }
  }
}