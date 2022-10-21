#include<stdio.h>
#include<mysql/plugin_audit.h>
#include"coordinator.h"
class Connection_event_handler{
public:
 void release_thd(MYSQL_THD THD);

void receive_event(MYSQL_THD THD, unsigned int event_class,
                          const void *event);
};
