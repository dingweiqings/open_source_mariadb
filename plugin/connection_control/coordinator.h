#define COORDINATOR_H
#include"data.h"
#include<iostream>

namespace connection_control{
class Lock;
class Connection_control_coordinator{

private:
    connection_control::Connection_control_variables g_variables;
    Lock* m_lock;
public:
    Connection_control_coordinator(int64 threshold,int64 maxDelay,int64 minDelay);
    bool coordinate(int64 failed_count,MYSQL_THD THD);
    connection_control::Connection_control_variables getGVariables(){
        return g_variables;
    }
    void condition_wait(MYSQL_THD thd,int64 time);
    void read_lock();
    void write_lock();
    void unlock();
};
}

