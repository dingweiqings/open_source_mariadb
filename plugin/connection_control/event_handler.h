#include<stdio.h>
#include<mysql/plugin_audit.h>
#include"coordinator.h"
namespace connection_control
{
extern int64 DEFAULT_THRESHOLD;
extern int64 DEFAULT_MIN_DELAY;
extern int64 DEFAULT_MAX_DELAY;
}; // namespace connection_control
class Connection_event_handler{
public:
 void release_thd(MYSQL_THD THD);

void receive_event(MYSQL_THD THD, unsigned int event_class,
                          const void *event);
};
