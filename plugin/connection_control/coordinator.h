#include"data.h"
#include<iostream>
#include"memory_store.h"
namespace connection_control
{
class Lock;
class Connection_control_coordinator
{

private:
  connection_control::Connection_control_variables g_variables;
  Lock *m_lock;

public:
  Connection_control_coordinator(int64 threshold, int64 maxDelay,
                                 int64 minDelay);
/**
 * @brief According to failed_count about  user@host , then decide if wait
 * 
 * @param failed_count 
 * @param THD 
 * @return true 
 * @return false 
 */
  bool coordinate(int64 failed_count, MYSQL_THD THD);
  connection_control::Connection_control_variables getGVariables()
  {
    return g_variables;
  }
  void read_lock();
  void write_lock();
  void unlock();
};
} // namespace connection_control
