#include <my_global.h>

#include "memory_store.h"
#include <limits.h>
namespace connection_control
{
/** Structure to maintain system variables */

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
};

/** Structure to maintain statistics */
class Connection_control_statistics
{
};
// class Connection{
//     private:
//     std:string m_username;
//     std:string m_host;
//     public:
//     std::string getUsername(){
//         return m_username;
//     }
//     std::string getHost(){
//         return m_host;
//     }
//         std::string getConnectionKey(){
//         return m_username+m_host;
//     }

// }

} // namespace connection_control
