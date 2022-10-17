#include"server.h"


bool is_have_task;
mutex my_mutex;
condition_variable semu_cond;

bool is_have_task1;
mutex my_mutex1;
condition_variable semu_cond1;


unordered_map<int, std::shared_ptr<clintchar>> account;  //��accountid������account����
mutex acc_mutex;
unordered_map<int, std::shared_ptr<clint>> cur_account_ptr;
mutex cur_account_ptr_mutex;

unordered_map<int, bool>islogin;//�����û�ָ���ٲ�
//�ͻ����Ǳߴ��ļ�Ӧ������һ���̲߳�Ӱ�����߳�ͨ��


unordered_map<int, queue<string>>handleingque;
mutex handle_acc_mutex;


void Server::clint_handle_accept(boost::system::error_code er, shared_ptr<clint>cl1) {
    if (!er) {
        cout << er.what() << endl;
    }
    cl.emplace_back(std::move(make_shared<clint>(ref(service))));
    cl.back()->this_it = cl.back();
    shared_ptr<clint>clptr = cl.back();
    clptr->id = curclintid++;
    acceptor->async_accept(clptr->sock(),
        boost::bind(&Server::clint_handle_accept,
            this,
            boost::asio::placeholders::error,
            clptr));
//#ifdef DEBUG
    cout << "�ͻ���id��" << cl1->id << "�ѽ���" << endl;
//#endif // DEBUG

    cl1->on_read();
}
void Server::gc() {
    while (1) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
        unique_lock<mutex>gc_lock(cur_account_ptr_mutex, std::defer_lock);
        if (gc_lock.try_lock()) {
#ifdef DEBUG
            cout << __LINE__ << "�õ���gc_lock" << endl;
#endif // DEBUG
            for (auto it = cur_account_ptr.begin(); it != cur_account_ptr.end(); ) {
                if ((*it).second)
                    if ((*it).second->isdiascard) {
                        cout << "id:" << (*it).second->id << "�ѱ�����" << endl;
                        cur_account_ptr.erase(it++);
                    }
                    else {
                        ++it;
                    }
                else {
                    ++it;
                }
            }
        }
        else {
#ifdef DEBUG
            cout << __LINE__ << "�ò�������gc_lock" << endl;
#endif // DEBUG
        }
    }
}
void Server::changestatus(string& p, unsigned int st) {
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}

void Server::tmphandlethread() {
    while (1) {
        unique_lock<mutex>sbguard(my_mutex);
        semu_cond.wait(sbguard, [this]() {
            if (is_have_task == true)return true;
            return false;
            });
#ifdef DEBUG
        cout << "�߳�tmphandlethread������" << endl;
#endif // DEBUG
        bool isCatchLock = true;
        for (auto& clchar : account) {//���������ü������ӳ�����

            string s;
            {//��semessage����
                //lock_guard<mutex>handle_lock((*it)->semu);
                std::unique_lock<std::mutex> handle_lock(clchar.second->semu, std::defer_lock);
                // print '*' if successfully locked, 'x' otherwise: 
#ifdef DEBUG
                cout << "�鿴�˻�" << clchar.first << "�Ĵ�ת����Ϣ" << endl;
#endif // DEBUG

                try {
                    if (handle_lock.try_lock()) {//�õ���//���û�õ�����߲�ֱ��˯���˰�
                        if (clchar.second->semessage.size() > 0) {
                            s = clchar.second->semessage.front();
                            clchar.second->semessage.pop();
#ifdef DEBUG
                            cout << "��ȡ������" << s << endl;
#endif // DEBUG                
                        }
                    }
                    else {
                        //this_thread::yield();
                        isCatchLock = false;
                    }
                }
                catch (std::exception& e) {
                    cout << e.what() << endl;
                }
            }
            {
                if (s.size() >= sizeof(Head)) {
                    //std::unique_lock<std::mutex> handle_lock(handle_acc_mutex, std::defer_lock);
                    Head h = getHead(s);                   
#ifdef DEBUG
                    cout << s << endl;
#endif // DEBUG
                    lock_guard<mutex>han_Lock(handle_acc_mutex);
                    handleingque[h.sendto].push(s);
#ifdef DEBUG
                    cout << "����" << h.sendto << "ת������" << endl;
#endif // DEBUG
                    is_have_task1 = true;
                    semu_cond1.notify_all();
                    //���⻽��ת���߳�//

                }
            }

        }
        if (isCatchLock)is_have_task = false;
        else is_have_task = true;//û�õ������ܻ�������Ҫ˯��
    }

}

void Server::handlequethread() {//�����ϣ������������߳�
    while (1) {
        unique_lock<mutex>sbguard1(my_mutex1);
        semu_cond1.wait(sbguard1, [this]() {
            if (is_have_task1 == true)return true;
            return false;
            });
        
        //��������������˾�˯�ߣ������һ��forѭ��δ����������Ͳ�˯
        bool isCatchLock = true;
        {
#ifdef DEBUG
            cout << __LINE__ << "****************************************��������" << endl;
#endif
            lock_guard<mutex>handleQueueLock(handle_acc_mutex);
#ifdef DEBUG
            cout <<__LINE__<< "****************************************�õ���" << endl;
#endif
#if 0
            for (auto& a : handleingque) {//�Ƿ�Ӧ������֮ǰ����?//Ҫ�ӣ�����������������Щ���⣬�ú�������ôд
                if (!a.second.empty()) {
                    {
                        unique_lock<mutex>loo(acc_mutex, std::defer_lock);
                        if (loo.try_lock()) {
                            if (account.find(a.first) == account.end()) {
                                handleingque.erase(a.first);
                                continue;//���û�и��˺�ֱ�Ӽ���
                            }
                                unique_lock<mutex>lo(account[a.first]->remu, std::defer_lock);
                            if (lo.try_lock()) { //û�õ�����˯���õ����˾�˵���ܰѵ�ǰ����˴����꣬������ж��õ�����һ���ܴ�����
#ifdef DEBUG
                                cout << "�õ���lo" << endl;
#endif
                                while (!a.second.empty()) {
#ifdef DEBUG
                                    cout << "ѹ����Ϣ:" << a.second.front() << endl;
#endif
                                    account[a.first]->remessage.push(a.second.front());
                                    a.second.pop();
                                }
                            }
                            else {
                                isCatchLock = false;//��������δ����
                            }
                        }
                    }
                }

#endif 
#if 1
            for (auto it = handleingque.begin(); it != handleingque.end(); ) {//�Ƿ�Ӧ������֮ǰ����?//Ҫ�ӣ�����������������Щ���⣬�ú�������ôд
                if (!it->second.empty()) {
                    {
                        unique_lock<mutex>loo(acc_mutex, std::defer_lock);
                        if (loo.try_lock()) {
                            if (account.find(it->first) == account.end()) {
                                handleingque.erase(it++);
                                //--it;
                                continue;//���û�и��˺�ֱ�Ӽ���
                            }
                            unique_lock<mutex>lo(account[it->first]->remu, std::defer_lock);
                            if (lo.try_lock()) { //û�õ�����˯���õ����˾�˵���ܰѵ�ǰ����˴����꣬������ж��õ�����һ���ܴ�����
#ifdef DEBUG
                                cout << "�õ���lo" << endl;
#endif
                                while (!it->second.empty()) {
#ifdef DEBUG
                                    cout << "ѹ����Ϣ:" << a.second.front() << endl;
#endif
                                    account[it->first]->remessage.push(it->second.front());
                                    it->second.pop();
                                }
                                it++;
                            }
                            else {
                                isCatchLock = false;//��������δ����
                                it++;
                            }
                        }
                        else {
                            it++;
                        }
                    }
                }
                else {
                    it++;
                }

            }
#endif
#ifdef DEBUG
            cout << __LINE__ << "****************************************�ͷ���" << endl;
#endif
        }
        if (isCatchLock) {
            is_have_task1 = false;

#ifdef DEBUG
            cout << "���������" << endl;
#endif
        }
        else {
            is_have_task1 = true;
            
#ifdef DEBUG
            cout << "����δ����" << endl;
#endif
        }
    }
}
void Server::handle_login_write() {
#ifdef DEBUG
    cout <<__LINE__<< "handle_login_write��������" << endl;
#endif
    while (1) {
        
#ifdef DEBUG
        cout << __LINE__ << "handle_login_write��������" << endl;
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(10));//˯��10����
#ifdef DEBUG
        cout << __LINE__ << "handle_login_write��������" << endl;
#endif
        unique_lock<mutex>ll(cur_account_ptr_mutex, std::defer_lock);
        if (ll.try_lock()) {
#ifdef DEBUG
            cout << "�õ���ll" << endl;
#endif
            //���ÿһ�������û��費��Ҫ����write
            for (auto &a : cur_account_ptr) {
#ifdef DEBUG
                cout << __LINE__ << "forѭ��������" << endl;
#endif // DEBUG
                if (cur_account_ptr[a.first] && !cur_account_ptr[a.first]->isdiascard) {//�����ָ�벻Ϊ��˵����ǰ�û�����
#ifdef DEBUG
                    cout << __LINE__ << "�û�" <<a.first<<"����" << endl;
#endif // DEBUG
                    unique_lock<mutex>lll(cur_account_ptr[a.first]->clch->remu, std::defer_lock);//������ն��п��Է���   
#ifdef DEBUG
                    cout << __LINE__ << "handle_login_write��������" << endl;
#endif
                    if (lll.try_lock()) {
#ifdef DEBUG
                        cout << __LINE__ << "�õ���lll" << endl;
#endif // DEBUG
                        if (cur_account_ptr[a.first]->clch->remessage.size() > 0) {
#ifdef DEBUG
                            cout << __LINE__ << "handle_login_write��������" << endl;
#endif                      
                            lll.unlock();//�Ƚ���Ҫ��on_write�Ǳ�û��ȡ����
                            cur_account_ptr[a.first]->on_write();//�����ǰҪ���͵��û�����

#ifdef DEBUG
                            cout << __LINE__ << "����****************************************************************" <<a.first<<"��on_write" << endl;
#endif // DEBUG
                        }
                        else {
#ifdef DEBUG
                            cout << __LINE__ << "δ����" << a.first << "��on_write" << endl;
#endif // DEBUG

                        }
                    }
                    else {
#ifdef DEBUG
                        cout << __LINE__ << "�ò�����lll" << endl;
#endif // DEBUG
                    }
                }
            }
        }
        else {
#ifdef DEBUG
            cout << __LINE__ << "�ò�����ll" << endl;
#endif // DEBUG
        }
    }
}