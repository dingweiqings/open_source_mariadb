#include <my_global.h>

#include "memory_store.h"
#include <limits.h>
namespace connection_control
{
int64 DEFAULT_MIN_DELAY=1000;   
int64 MIN_DELAY=1000;


int64 DEFAULT_MAX_DELAY=INT64_MAX;   
int64 MAX_DELAY=INT64_MAX;


int64 DEFAULT_THRESHOLD=3;
int64 MIN_THRESHOLD=3;
int64 MAX_THRESHOLD=1000;

class Connection_control_variables
{
private:
  /* Various global variables */
  int64 failed_connections_threshold;
  int64 min_connection_delay;
  int64 max_connection_delay;

public:
  Connection_control_variables(int64 threshold, int64 minDelay, int64 maxDelay)
  {
    failed_connections_threshold= threshold;
    min_connection_delay= minDelay;
    max_connection_delay= maxDelay;
  }
  Connection_control_variables() {}
  int64 getFailedConnectionsThreshold()
  {
    return failed_connections_threshold;
  }
  int64 getMinDelay() { return min_connection_delay; }
  int64 getMaxDelay() { return max_connection_delay; }

  void setFailedConnectionsThreshold(int64 value)
  {
    failed_connections_threshold= value;
  }
  void setMinDelay(int64 value) { min_connection_delay= value; }
  void setMaxDelay(int64 value) { max_connection_delay= value; }
}
};
