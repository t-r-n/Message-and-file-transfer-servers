#include"con_manager.h"

void con_manager::start(FileClint_ptr ptr) {
	cl.insert(ptr);
	cout << "开始do——read" << endl;
	ptr->do_read();
}
void con_manager::gc() {
    cout << "开始gc" << endl;
    while (1) {
        cout << "调用一次gc" << endl;
        unique_lock<mutex>clguard(cl_mutex);
        cl_mutex_cond.wait(clguard, [this]() {
            if (is_end.load() == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "拿到锁gc_lock" << endl;
#endif // DEBUG
        for (auto it = cl.begin(); it != cl.end(); ) {
            if ((*it))
                if ((*it)->is_delete.load()) {
                    cout<<"已被清理" << endl;
                    cl.erase(it++);
                }
                else {
                    ++it;
                }
            else {
                ++it;
            }
        }
        is_end.store(false);
    }
}