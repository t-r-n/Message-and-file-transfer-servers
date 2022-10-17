#include"clint.h"

void clint::on_write1(int back) {
    if (isdiascard) {
        return;
    }
    he.type = 'r';//��������Ϣ����
    he.status = back;
    he.length = 0;
    memcpy(ttmphead, &he, sizeof(Head));
    on_write1_buf = string(ttmphead, sizeof(Head));
    async_write(sock_, buffer(on_write1_buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
        //async_write(sock_, buffer(buf1), [=, this](boost::system::error_code er, size_t size) {
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << this->id << ":���ӹر�" << endl;

                sock_.close();
                islogout = true;
            }
            else {
                on_write1(reback::writeerror);
            }
            /*        cout << "async_write error" << endl;
                    sock_.close();
                    islogout = true;*/

            return;
        }
        cout << "send ed:" << he.type << " " << he.status << " " << he.length << endl;
    });
}

void clint::on_read() {
    if (isdiascard) {
        return;
    }
    buf.clear();  //��������ߵ��µĶ�������
    buf.resize(1024);//����������async_read��һֱ��ȡ
    auto self(shared_from_this());
    //����������������յ����ܽ�����ͷ������
    //async_read(sock_, buffer(buf), transfer_at_least(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        async_read(sock_, buffer(buf), transfer_at_least(1), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {

        //async_read(sock_, buffer(buf),"\n", [=](boost::system::error_code er, size_t sz) {
            //transfer_all()��ֱ����ȡ��1024�ֽ�Ҳ����˵��buffer�����ŷ���
            //transfer_at_least(n)������������n�ֽ�ʱ��ȡ����������ܵ����ݵĻ����͵�ʱ�򻺳�������Ŀ��ַ�Ҳ����ȥ����Ҫ����һ�£�Ҳ����˵Ҫ�Լ�����ÿ��
            //ÿ�η�����Ľ�β��ǻ�涨һ�����ʵ�Ҫ���͵Ļ�������Сbuffer(buf,���ʵĴ�С)
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "���ӹر�" << endl;//�������
                sock_.close();
                islogout = true;
            }
            else {
                on_write1(reback::readerror);
                self->on_read();
            }
            return;
        }
        cout << "���յ��Ķ�����" << buf << endl;
        string headme(buf.begin(), buf.begin() + sizehead);
        memcpy(ttmphead, headme.c_str(), sizehead);
        h = (Head*)ttmphead;
        int ret = headanylize(h);//0����Ϣͷ��ʽ����1��Ϣ��2�ļ�����
        if (ret == 1) {//��Ϣ

#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "���յ���Ϣ�ĳ���" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "���յ�����Ϣ" << tmp << endl;
#endif
            //��Ϣ/�ļ����͸�˭�Ƿŵ���ͷ�ﻹ������Ϣ��//���ڰ�������񻹵����´����²��ܷŻ���Ϣ�����������߲�����ֱ���öԷ�on_write�Ǳߴ���ֱ��ѹ��������
            //��ǰ�汾�ǲ�����  //�����ǲ�����server�Ǳ߰Ѱ�ͷ��΢��һ��ֱ�ӷ����Է��Ͼ��Է����ܵ�����ҲӦ���и��ְ�ͷ
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else if (ret == 2) {//�ļ�����
#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "���յ���Ϣ�ĳ���" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "���յ�����Ϣ" << tmp << endl;
#endif
            //��Ϣ/�ļ����͸�˭�Ƿŵ���ͷ�ﻹ������Ϣ��//���ڰ�������񻹵����´����²��ܷŻ���Ϣ�����������߲�����ֱ���öԷ�on_write�Ǳߴ���ֱ��ѹ��������
            //��ǰ�汾�ǲ�����  //�����ǲ�����server�Ǳ߰Ѱ�ͷ��΢��һ��ֱ�ӷ����Է��Ͼ��Է����ܵ�����ҲӦ���и��ְ�ͷ
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);

                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else if (ret == 3) {//��¼����
            unsigned int tmppassword = 0;
            {
                lock_guard<mutex>acc_lock(acc_mutex);
                if (account.find(h->account) != account.end()) {//�и��˻�
                    tmppassword = account[h->account]->password;
                }
                else {
                    on_write1(reback::loginfal);//��½ʧ��
                    self->on_read();
                    return;
                }
            }
            if (tmppassword != h->mima) {//���벻��ȷ;
                this->on_write1(reback::passerror);
            }
            else {//��½�ɹ�//��ȫ�ֵ�clint����һ��     ///*************�����ִ��������ߵ�accountռ�Ź�ϣ�����Դ���ǲ����ʵ�

                //������û���Ѿ�����
                bool isloogin = false;
                {
                    lock_guard<mutex>cur_log_lock(cur_account_ptr_mutex);
#ifdef DEBUG
                    cout << __LINE__ << "�õ���cur_log_lock" << endl;
#endif // DEBUG
                    if (cur_account_ptr.find(h->account) == cur_account_ptr.end()) {
                        cur_account_ptr[h->account] = this_it;
                    }
                    else {
                        if (cur_account_ptr[h->account]) {
                            if (cur_account_ptr[h->account]->isdiascard) {
                                cur_account_ptr[h->account] = this_it;
                            }
                            else {
                                isloogin = true;
                            }
                        }
                        else {
                            cur_account_ptr[h->account] = this_it;
                        }
                    }
                }
                if (isloogin) {
                    on_write1(reback::login);
                    //�Ͽ�������
                }
                else {
                    clch = account[h->account];  //֮����һ�����account���sharer����ptr�������cur_ptr��û�г�ͻ
                    on_write1(reback::logsucc);
                    cout << "��½�ɹ�" << endl;
                }
            }
        }
        else if (ret == 4) {//ע��newһ��clintchar
            clch = make_shared<clintchar>();
            bool issuc = false;
            {
                lock_guard<mutex>login_lock(acc_mutex);
                if (account.find(h->account) == account.end()) {
                    account[h->account] = clch;
                    issuc = true;
                }
                else {
                    on_write1(reback::loginfal);
                    cout << "ע��ʧ��" << endl;
                }
            }
            if (issuc) {
                clch->account = h->account;
                clch->password = h->mima;
                on_write1(reback::loginsucc);
                cout << "ע��ɹ�" << endl;
            }
        }
        else if (ret == 5) {//�ͻ��������ļ�id

        }
        else if (ret == 6) {//�ѵ�ǰӵ�е��ļ��ļ������ͻ�ȥ
   //���ڲ��ҵľ��
#ifdef WIN
            string eassy;
            bool is = true;
            try {
                long handle;
                struct _finddata_t fileinfo;
                //��һ�β���
                handle = _findfirst(inPath.c_str(), &fileinfo);
                do
                {
                    //�ҵ����ļ����ļ���
                    if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
                    eassy += string(fileinfo.name) + "\n";
                    //printf("%s\n", fileinfo.name);

                } while (!_findnext(handle, &fileinfo));
                _findclose(handle);
            }
            catch (...) {
                is = false;
                on_write1(reback::readerror);
                cout << "��ȡ�����ļ�error" << endl;
            }
            if (is) {
                h->type = 'd';
                h->length = eassy.size();
                h->sendto = h->account;
                h->account = 0;
            }
            string bufd = string((char*)h, sizeof(Head)) + eassy;
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(bufd);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
                }
            }
#endif //WIN
        }
        else if (ret==7) {
#ifdef DEBUG
        cout << "�������" << endl;
        unsigned int leng = ((Head*)ttmphead)->length;
        cout << "���յ���Ϣ�ĳ���" << leng << endl;
        string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
        cout << "���յ�����Ϣ" << tmp << endl;
#endif // DEBUG
        

                async_write(sock_, buffer(buf,buf.size()), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                        if (er) {
                            int ret = errorhandle(er);
                            if (ret == 0) {
                                cout << this->id << ":���ӹر�" << endl;
                                sock_.close();
                                islogout = true;
                            }
                            else {
                                on_write1(reback::writeerror);
                            }
                            return;
                        }
                });
        }
        else if (ret == 8) {
        
            {
                shared_ptr<clintchar>pr;
                {
                    lock_guard<mutex>ll(acc_mutex);
                    pr = account[h->account];
                }
                    lock_guard<mutex> selock(pr->semu);
                    pr->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//���Ѵ���������߳�
            }//���ֻ���ܰѸ���Ϣ���͵�������������Է������߻����崻�����������ݽ����Է����ϣ����ֻ�ܱ�֤���������һ���ɹ������Է���
        }
        else {
                async_write(sock_, buffer(buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                    if (er) {
                        int ret = errorhandle(er);
                        if (ret == 0) {
                            cout << this->id << ":���ӹر�" << endl;
                            sock_.close();
                            islogout = true;
                        }
                        else {
                            on_write1(reback::writeerror);
                        }
                        return;
                    }
                    //cout << "send ed:" << he.type << " " << he.status << " " << he.length << endl;
                });
            
            //on_write1(reback::readerror);
#ifdef DEBUG
                cout << "�޷�������ͷ�Ǳ�׼�ͻ��˷��͵���Ϣ" << endl;
#endif // DEBUG
        }
        self->on_read();
        }));
}

void clint::on_write() {
    if (isdiascard) {
        return;
    }
    reme.clear();
    reme.resize(1024);
    {
        lock_guard<mutex> relock(clch->remu);
#ifdef DEBUG
        cout << __LINE__ << "�õ���" << endl;
#endif // DEBUG

        if (clch->remessage.size() > 0) {
            reme = clch->remessage.front();
            //����Ϣ״̬��Ϊ�Ѵ���
            clch->remessage.pop();
        }
        else {
            return;//���һ����֤��ֹ���ֲ�ȷ����
        }
    }
#ifdef DEBUG
    cout << "id:" << this->id << ":" << this->clch->account << "��ʼ�첽��д" << reme << endl;
#endif
    async_write(sock_, buffer(reme, reme.size()), transfer_at_least(reme.size() - 1), bind(&clint::clint_handle_write, this, reme, _1, _2));//���ռλ�����ܻ���Щ����
}
void clint::clint_handle_write(string p, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        int ret = errorhandle(er);
        if (ret == 0) {
            cout << "���ӹر�" << endl;
            sock_.close();
            islogout = true;
        }
        else {
            on_write1(reback::writeerror);
        }
        //������Ϣ������Ϊ��ʼ̬ѹ�����
        changestatus(p, 0);
        {
            lock_guard<mutex>relock(clch->remu);
#ifdef DEBUG
            cout << __LINE__ << "�õ���" << endl;
#endif // DEBUG
            clch->remessage.push(p);
        }
        cout << "on_write error";
        return;
    }
    //����Ϣ״̬��Ϊ�ɶ���ѹ�����
    this->on_write();
}
int clint::headanylize(Head*& head) {
    if (head->type == 'm') {
        return 1;
    }
    else if (head->type == 'f') {
        return 2;
    }
    else if (head->type == 'l') {//��¼
        return 3;
    }
    else if (head->type == 'i') {//ע��
        return 4;
    }
    else if (head->type == 'a') {
        return 5;
    }
    else if (head->type == 'd') {//��ȡ��������ǰӵ�е��ļ�
        return 6;
    }
    else if (head->type == 'r') {//�������
        return 7;
    }
    else if (head->type == 'z') {//�������
        return 8;
    }
    else {
        return 0;//�޷�������ͷ�ͻ�����Ϣ
    }
}
void clint:: changestatus(string& p, unsigned int st) {//��ߴ������п����ַ�������˵Ȼ���bug��������
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
int clint::errorhandle(boost::system::error_code& er) {
    if (isdiascard) {//�������첽�����Ѿ������ö��߱�������
        return 0;
    }
    int ervalue = er.value();
    if (ervalue == 10009 || ervalue == 2 || ervalue == 995 || ervalue == 10054 || ervalue == 2) {//
        isdiascard = true;
        return 0;
    }
    return 1;  //һ�����
}

ip::tcp::socket& clint::sock() {
    return sock_;
}