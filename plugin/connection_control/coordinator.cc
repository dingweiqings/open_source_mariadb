#include "coordinator.h"
#include "my_sys.h"
#include "my_pthread.h"
#include "mysql/psi/mysql_thread.h"
#include "mysql/psi/psi.h"

extern PSI_mutex_key key_connection_delay_mutex;
extern PSI_rwlock_key key_connection_event_delay_lock;
extern PSI_cond_key key_connection_delay_wait;
extern PSI_stage_info stage_waiting_in_connection_control_plugin;
extern Memory_store<std::string, int64>* data_store;
/* Forward declaration */
void thd_enter_cond(MYSQL_THD thd, mysql_cond_t *cond, mysql_mutex_t *mutex,
                    const PSI_stage_info *stage, PSI_stage_info *old_stage,
                    const char *src_function, const char *src_file,
                    int src_line);
void thd_exit_cond(MYSQL_THD thd, const PSI_stage_info *stage,
                   const char *src_function, const char *src_file,
                   int src_line);

namespace connection_control
{
/** Helper lock class */
class Lock
{
private:
  mysql_rwlock_t m_lock;

public:
  Lock() { mysql_rwlock_init(key_connection_event_delay_lock, &m_lock); }
  ~Lock() { mysql_rwlock_destroy(&m_lock); }
  void read_lock() { mysql_rwlock_rdlock(&m_lock); }
  void write_lock() { mysql_rwlock_wrlock(&m_lock); }
  void unlock() { mysql_rwlock_unlock(&m_lock); }
};
/**
 * @brief  Use conditional variables to implement delay
 *
 * @param thd
 * @param time
 */
void condition_wait(MYSQL_THD thd, int64 time)
{
  /** mysql_cond_timedwait requires wait time in timespec format */
  struct timespec abstime;
  /** Since we get wait_time in milliseconds, convert it to nanoseconds */
  set_timespec_nsec(abstime, time * 1000000);

  /** PSI_stage_info for thd_enter_cond/thd_exit_cond */
  PSI_stage_info old_stage;
  /** Initialize mutex required for mysql_cond_timedwait */
  mysql_mutex_t connection_delay_mutex;
  mysql_mutex_init(key_connection_delay_mutex, &connection_delay_mutex,
                   MY_MUTEX_INIT_FAST);

  /* Initialize condition to wait for */
  mysql_cond_t connection_delay_wait_condition;
  mysql_cond_init(key_connection_delay_wait, &connection_delay_wait_condition,
                  NULL);

  /** Register wait condition with THD */
  mysql_mutex_lock(&connection_delay_mutex);

  thd_enter_cond(thd, &connection_delay_wait_condition,
                 &connection_delay_mutex,
                 &stage_waiting_in_connection_control_plugin, &old_stage,
                 __func__, __FILE__, __LINE__);

  /*
    At this point, thread is essentially going to sleep till
    timeout. If admin issues KILL statement for this THD,
    there is no point keeping this thread in sleep mode only
    to wake up to be terminated. Hence, in case of KILL,
    we will return control to server without worring about
    wait_time.
  */
  mysql_cond_timedwait(&connection_delay_wait_condition,
                       &connection_delay_mutex, &abstime);

  /**
   * @brief Exit_cond will release mutex
   *
   */
  thd_exit_cond(thd, &stage_waiting_in_connection_control_plugin, __func__,
                __FILE__, __LINE__);

  /* Cleanup */
  mysql_mutex_destroy(&connection_delay_mutex);
  mysql_cond_destroy(&connection_delay_wait_condition);
}

static Lock lock;

Connection_control_coordinator::Connection_control_coordinator(int64 threshold,
                                                               int64 minDelay,
                                                               int64 maxDelay)
{
  g_variables.setFailedConnectionsThreshold(threshold);
  g_variables.setMaxDelay(maxDelay);
  g_variables.setMinDelay(minDelay);
  m_lock= &lock;
}

bool Connection_control_coordinator::coordinate(int64 failed_count,
                                                MYSQL_THD THD)
{
  // Failed up to threshold
  if (failed_count > g_variables.getFailedConnectionsThreshold())
  {
    // TODO add coordinator strategy
    std::cout << "Now sleep 5s" << std::endl;
    condition_wait(THD, 5000);
    std::cout << "Sleep 5s end" << std::endl;
  }
  return true;
}
void Connection_control_coordinator::read_lock() { m_lock->read_lock(); }
void Connection_control_coordinator::write_lock() { m_lock->write_lock(); }
void Connection_control_coordinator::unlock() { m_lock->unlock(); }
} // namespace connection_control
