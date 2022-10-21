#include"data.h"
#include<iostream>
#include"lock.h"
#include <my_global.h>
extern Memory_store<std::string,uint64> data_store;
Connection_control_coordinator coordinator=Connection_control_coordinator(10,100,1);
connection_control::Lock lock;
class Connection_control_coordinator{

private:
    connection_control::Connection_control_variables g_variables;
public:
    Connection_control_coordinator(uint64 threshold,uint64 minDelay,uint64 maxDelay){
        g_variables.setFailedConnectionsThreshold(threshold);
        g_variables.setMaxDelay(maxDelay);
        g_variables.setMinDelay(minDelay);
    }
    bool coordinate(MYSQL_THD THD,std::string connectionKey){
        uint64 failed_count=data_store.find(connectionKey);
        if(failed_count > g_variables.getFailedConnectionsThreshold()){
                //
                lock.condition_wait(THD,1000);
                std::cout<<"Should sleep"<<std::endl;
        }
        return true;
    }

};