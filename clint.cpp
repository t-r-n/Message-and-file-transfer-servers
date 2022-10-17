#include"clint.h"

void clint::on_write1(int back) {
    if (isdiascard) {
        return;
    }
    he.type = 'r';//服务器消息回馈
    he.status = back;
    he.length = 0;
    memcpy(ttmphead, &he, sizeof(Head));
    on_write1_buf = string(ttmphead, sizeof(Head));
    async_write(sock_, buffer(on_write1_buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
        //async_write(sock_, buffer(buf1), [=, this](boost::system::error_code er, size_t size) {
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << this->id << ":链接关闭" << endl;

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
    buf.clear();  //好像是这边导致的断言问题
    buf.resize(1024);//不加这两行async_read会一直读取
    auto self(shared_from_this());
    //如果服务器不允许收到不能解析包头的数据
    //async_read(sock_, buffer(buf), transfer_at_least(sizehead), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {
        async_read(sock_, buffer(buf), transfer_at_least(1), strand_.wrap([this, self](boost::system::error_code er, size_t sz) {

        //async_read(sock_, buffer(buf),"\n", [=](boost::system::error_code er, size_t sz) {
            //transfer_all()会直到读取到1024字节也就是说把buffer读满才返回
            //transfer_at_least(n)当缓冲区大于n字节时读取但不处理接受的数据的话发送的时候缓冲区后面的空字符也发过去了需要处理一下，也就是说要自己定义每次
            //每次发送完的结尾标记或规定一个合适的要发送的缓冲区大小buffer(buf,合适的大小)
        if (er) {
            int ret = errorhandle(er);
            if (ret == 0) {
                cout << "链接关闭" << endl;//网络错误
                sock_.close();
                islogout = true;
            }
            else {
                on_write1(reback::readerror);
                self->on_read();
            }
            return;
        }
        cout << "接收到的东西：" << buf << endl;
        string headme(buf.begin(), buf.begin() + sizehead);
        memcpy(ttmphead, headme.c_str(), sizehead);
        h = (Head*)ttmphead;
        int ret = headanylize(h);//0，消息头格式错误，1消息，2文件传输
        if (ret == 1) {//消息

#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "接收到消息的长度" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "接收到的消息" << tmp << endl;
#endif
            //消息/文件发送给谁是放到包头里还是在消息里//放在包里这好像还得重新处理下才能放回消息队列里或者这边不处理直接让对方on_write那边处理直接压倒对列里
            //当前版本是不处理  //必须是不处理server那边把包头稍微改一下直接发给对方毕竟对方接受的数据也应该有各种包头
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);
                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else if (ret == 2) {//文件传输
#ifdef DEBUG
            unsigned int leng = ((Head*)ttmphead)->length;
            cout << "接收到消息的长度" << leng << endl;
            string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
            cout << "接收到的消息" << tmp << endl;
#endif
            //消息/文件发送给谁是放到包头里还是在消息里//放在包里这好像还得重新处理下才能放回消息队列里或者这边不处理直接让对方on_write那边处理直接压倒对列里
            //当前版本是不处理  //必须是不处理server那边把包头稍微改一下直接发给对方毕竟对方接受的数据也应该有各种包头
            {
                if (clch) {
                    lock_guard<mutex> selock(clch->semu);
                    clch->semessage.push(buf);

                    is_have_task = true;//***********************************************************************************************************************
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else if (ret == 3) {//登录环节
            unsigned int tmppassword = 0;
            {
                lock_guard<mutex>acc_lock(acc_mutex);
                if (account.find(h->account) != account.end()) {//有该账户
                    tmppassword = account[h->account]->password;
                }
                else {
                    on_write1(reback::loginfal);//登陆失败
                    self->on_read();
                    return;
                }
            }
            if (tmppassword != h->mima) {//密码不正确;
                this->on_write1(reback::passerror);
            }
            else {//登陆成功//把全局的clint拷贝一份     ///*************当出现大量已下线的account占着哈希表的资源这是不合适的

                //看看有没有已经在线
                bool isloogin = false;
                {
                    lock_guard<mutex>cur_log_lock(cur_account_ptr_mutex);
#ifdef DEBUG
                    cout << __LINE__ << "拿到锁cur_log_lock" << endl;
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
                    //断开链接吗？
                }
                else {
                    clch = account[h->account];  //之后检查一下这个account里的sharer——ptr和上面的cur_ptr有没有冲突
                    on_write1(reback::logsucc);
                    cout << "登陆成功" << endl;
                }
            }
        }
        else if (ret == 4) {//注册new一个clintchar
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
                    cout << "注册失败" << endl;
                }
            }
            if (issuc) {
                clch->account = h->account;
                clch->password = h->mima;
                on_write1(reback::loginsucc);
                cout << "注册成功" << endl;
            }
        }
        else if (ret == 5) {//客户端请求文件id

        }
        else if (ret == 6) {//把当前拥有的文件文件名发送回去
   //用于查找的句柄
#ifdef WIN
            string eassy;
            bool is = true;
            try {
                long handle;
                struct _finddata_t fileinfo;
                //第一次查找
                handle = _findfirst(inPath.c_str(), &fileinfo);
                do
                {
                    //找到的文件的文件名
                    if ((!strcmp(fileinfo.name, ".")) || !strcmp(fileinfo.name, ".."))continue;
                    eassy += string(fileinfo.name) + "\n";
                    //printf("%s\n", fileinfo.name);

                } while (!_findnext(handle, &fileinfo));
                _findclose(handle);
            }
            catch (...) {
                is = false;
                on_write1(reback::readerror);
                cout << "获取本地文件error" << endl;
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
                    semu_cond.notify_all();//唤醒处理任务的线程
                }
            }
#endif //WIN
        }
        else if (ret==7) {
#ifdef DEBUG
        cout << "回射服务" << endl;
        unsigned int leng = ((Head*)ttmphead)->length;
        cout << "接收到消息的长度" << leng << endl;
        string tmp(buf.begin() + sizeofhead, buf.begin() + (sizeofhead + leng));
        cout << "接收到的消息" << tmp << endl;
#endif // DEBUG
        

                async_write(sock_, buffer(buf,buf.size()), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                        if (er) {
                            int ret = errorhandle(er);
                            if (ret == 0) {
                                cout << this->id << ":链接关闭" << endl;
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
                    semu_cond.notify_all();//唤醒处理任务的线程
            }//这个只是能把该消息发送到服务器上如果对方不在线或服务宕机都不会把数据交到对方手上，这边只能保证服务器这边一定成功发给对方了
        }
        else {
                async_write(sock_, buffer(buf), [=, self = shared_from_this()](boost::system::error_code er, size_t size) {
                    if (er) {
                        int ret = errorhandle(er);
                        if (ret == 0) {
                            cout << this->id << ":链接关闭" << endl;
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
                cout << "无法解析包头非标准客户端发送的消息" << endl;
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
        cout << __LINE__ << "拿到锁" << endl;
#endif // DEBUG

        if (clch->remessage.size() > 0) {
            reme = clch->remessage.front();
            //将消息状态置为已处理
            clch->remessage.pop();
        }
        else {
            return;//多加一层认证防止各种不确定性
        }
    }
#ifdef DEBUG
    cout << "id:" << this->id << ":" << this->clch->account << "开始异步读写" << reme << endl;
#endif
    async_write(sock_, buffer(reme, reme.size()), transfer_at_least(reme.size() - 1), bind(&clint::clint_handle_write, this, reme, _1, _2));//这边占位符可能还有些问题
}
void clint::clint_handle_write(string p, boost::system::error_code er, size_t sz) {
    if (isdiascard) {
        return;
    }
    if (er) {
        int ret = errorhandle(er);
        if (ret == 0) {
            cout << "链接关闭" << endl;
            sock_.close();
            islogout = true;
        }
        else {
            on_write1(reback::writeerror);
        }
        //出错将消息重新置为初始态压入队列
        changestatus(p, 0);
        {
            lock_guard<mutex>relock(clch->remu);
#ifdef DEBUG
            cout << __LINE__ << "拿到锁" << endl;
#endif // DEBUG
            clch->remessage.push(p);
        }
        cout << "on_write error";
        return;
    }
    //将消息状态置为可丢弃压入队列
    this->on_write();
}
int clint::headanylize(Head*& head) {
    if (head->type == 'm') {
        return 1;
    }
    else if (head->type == 'f') {
        return 2;
    }
    else if (head->type == 'l') {//登录
        return 3;
    }
    else if (head->type == 'i') {//注册
        return 4;
    }
    else if (head->type == 'a') {
        return 5;
    }
    else if (head->type == 'd') {//获取服务器当前拥有的文件
        return 6;
    }
    else if (head->type == 'r') {//回射服务
        return 7;
    }
    else if (head->type == 'z') {//回射服务
        return 8;
    }
    else {
        return 0;//无法解析包头就回射消息
    }
}
void clint:: changestatus(string& p, unsigned int st) {//这边处理完有可能字符串搞错了等会修bug考虑这里
    string headme(p.begin(), p.begin() + sizeof(Head));
    memcpy(ttmphead, headme.c_str(), sizeof(Head));
    //Head h = *(Head*)ttmphead;
    h = (Head*)ttmphead;
    h->status = st;
    p = string(ttmphead, sizeof(Head)) + string(p.begin() + sizeof(Head), p.end());
}
int clint::errorhandle(boost::system::error_code& er) {
    if (isdiascard) {//有其他异步任务已经检测出该对线被丢弃了
        return 0;
    }
    int ervalue = er.value();
    if (ervalue == 10009 || ervalue == 2 || ervalue == 995 || ervalue == 10054 || ervalue == 2) {//
        isdiascard = true;
        return 0;
    }
    return 1;  //一般错误
}

ip::tcp::socket& clint::sock() {
    return sock_;
}