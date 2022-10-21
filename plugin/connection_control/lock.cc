#include"lock.h"

namespace connection_control{

    
//  Lock::Lock()   {
//     mysql_rwlock_init(key_connection_event_delay_lock,&m_lock);
 
//  }
//  Lock::~Lock(){
//     mysql_rwlock_destroy(&m_lock);
//  }
// void Lock::rw_lock(){
//     mysql_rwlock_wrlock(&m_lock);
// }
// void Lock::unlock(){
//     mysql_rwlock_unlock(&m_lock);
// }
void Lock::condition_wait(MYSQL_THD thd,u_int64_t time){
      /** mysql_cond_timedwait requires wait time in timespec format */
  struct timespec abstime;
  /** Since we get wait_time in milliseconds, convert it to nanoseconds */
//   set_timespec_nsec(&abstime, wait_time * 1000000ULL);

  /** PSI_stage_info for thd_enter_cond/thd_exit_cond */
  //PSI_stage_info old_stage;

  /** Initialize mutex required for mysql_cond_timedwait */
  mysql_mutex_t connection_delay_mutex;
  mysql_mutex_init(key_connection_delay_mutex, &connection_delay_mutex,
                   NULL);

  // /* Initialize condition to wait for */
  // mysql_cond_t connection_delay_wait_condition;
  // mysql_cond_init( key_connection_delay_wait,&connection_delay_wait_condition,NULL);

  /** Register wait condition with THD */
 // mysql_mutex_lock(&connection_delay_mutex);

//   THD_ENTER_COND(thd, &connection_delay_wait_condition, &connection_delay_mutex,
//                  &stage_waiting_in_connection_control_plugin, &old_stage,
//                  __func__, __FILE__, __LINE__);

  /*
    At this point, thread is essentially going to sleep till
    timeout. If admin issues KILL statement for this THD,
    there is no point keeping this thread in sleep mode only
    to wake up to be terminated. Hence, in case of KILL,
    we will return control to server without worring about
    wait_time.
  */
  // mysql_cond_timedwait(&connection_delay_wait_condition,
  //                      &connection_delay_mutex, &abstime);

  /* Finish waiting and deregister wait condition */
//  mysql_mutex_unlock(&connection_delay_mutex);

//   THD_EXIT_COND(thd, &stage_waiting_in_connection_control_plugin, __func__,
//                 __FILE__, __LINE__);

  /* Cleanup */
  // mysql_mutex_destroy(&connection_delay_mutex);
  // mysql_cond_destroy(&connection_delay_wait_condition);
}



}