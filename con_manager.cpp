#include"con_manager.h"

void con_manager::start(FileClint_ptr ptr) {
	cl.insert(ptr);
	cout << "��ʼdo����read" << endl;
	ptr->do_read();
}
void con_manager::gc() {
    cout << "��ʼgc" << endl;
    while (1) {
        cout << "����һ��gc" << endl;
        unique_lock<mutex>clguard(cl_mutex);
        cl_mutex_cond.wait(clguard, [this]() {
            if (is_end.load() == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << __LINE__ << "�õ���gc_lock" << endl;
#endif // DEBUG
        for (auto it = cl.begin(); it != cl.end(); ) {
            if ((*it))
                if ((*it)->is_delete.load()) {
                    cout<<"�ѱ�����" << endl;
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