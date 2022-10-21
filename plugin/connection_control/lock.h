#include<mysql/psi/mysql_thread.h>
#include <my_global.h>
namespace connection_control {

class Lock {
private:
mysql_rwlock_t m_lock;
mysql_cond_t m_cond;
PSI_mutex_key key_connection_delay_mutex;
PSI_rwlock_key key_connection_event_delay_lock;
PSI_cond_key key_connection_delay_wait;
PSI_stage_info stage_waiting_in_connection_control_plugin;
public:
Lock();
~Lock();
void rw_lock();
void condition_wait(MYSQL_THD thd,uint64 time);
void unlock();
};
}  // namespace connection_control