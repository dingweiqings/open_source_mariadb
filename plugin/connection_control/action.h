#include <my_global.h>
class Connection_control_action{
    private:
    mysql_lock lock;
    public:
        void condition_wait(uint32 time);
}
