#include <my_global.h>
#include "memory_store.h"
#include<limits.h>
namespace connection_control {
/** Structure to maintain system variables */
uint64 MIN_DELAY=1;
uint64 MAX_DELAY=UINT64_MAX;
class Connection_control_variables {
 private:
  /* Various global variables */
  uint64 failed_connections_threshold;
  uint64 min_connection_delay;
  uint64 max_connection_delay;


  public:
  Connection_control_variables(int64 threshold,int64 minDelay,int64 maxDelay ){
    failed_connections_threshold=threshold;
    min_connection_delay=minDelay;
    max_connection_delay=maxDelay;
  }
    Connection_control_variables(){
        
    }
  uint64  getFailedConnectionsThreshold(){
    return failed_connections_threshold;
  }
  uint64  getMinDelay(){
    return min_connection_delay;
  }
  uint64  getMaxDelay(){
    return max_connection_delay;
  }

   void  setFailedConnectionsThreshold(uint64 value){
    failed_connections_threshold=value;
  }
  void  setMinDelay(uint64 value){
     min_connection_delay=value;
  }
  void setMaxDelay(uint64 value){
     max_connection_delay=value;
  }


};

/** Structure to maintain statistics */
class Connection_control_statistics {

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

}  // namespace connection_control
