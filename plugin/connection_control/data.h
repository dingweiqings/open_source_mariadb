#include <my_global.h>

namespace connection_control
{
class Connection_control_variables
{
public:
  /* Various global variables */
  int64 failed_connections_threshold;
  int64 min_connection_delay;
  int64 max_connection_delay;

public:
  Connection_control_variables() {}
  Connection_control_variables(int64 threshold, int64 min_delay,
                               int64 max_delay)
  {
    failed_connections_threshold= threshold;
    min_connection_delay= min_delay;
    max_connection_delay= max_delay;
  }
  int64 getFailedConnectionsThreshold()
  {
    return failed_connections_threshold;
  }
  int64 getMinDelay() { return min_connection_delay; }
  int64 getMaxDelay() { return max_connection_delay; }
};
}; // namespace connection_control
