//#pragma once
#ifndef SERVER_H
#define SERVER_H
#include "server_config.h"
#include"clint.h"
#include"FileServer.h"
class Server :public std::enable_shared_from_this<Server> {//服务器应该开一个线程池读取客户的消息队列调用相应客户的on_write函数
public:
    string buf;
    io_service service;
    typedef shared_ptr<ip::tcp::socket> sock_ptr;
    typedef boost::system::error_code error_code;
    sock_ptr sock;
    shared_ptr< ip::tcp::acceptor>acceptor;
    int curdfileid = 0; //描述如果当前加进来的是文件应该赋予的id
    int curclintid = 0;
    list<shared_ptr<clint>>cl;
    shared_ptr< ilovers::TaskExecutor>executor;
    Server() {
        sock_ptr sock1(new ip::tcp::socket(service));
        sock = sock1;
        boost::asio::io_service::work work(service);//不让run退出
        sock->open(ip::tcp::v4());
        acceptor = shared_ptr< ip::tcp::acceptor>(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8006)));
        cl.emplace_back(std::move(make_shared<clint>(ref(service))));
        cl.back()->this_it = cl.back();
        shared_ptr<clint>clptr = cl.back();
        clptr->id = curclintid++;
        error_code error;
        acceptor->async_accept(clptr->sock(),
            boost::bind(&Server::clint_handle_accept,
                this,
                boost::asio::placeholders::error,
                clptr));
        
        //vector<thread>thve;
        //thve.emplace_back(thread(&Server::tmphandlethread, this));
        //thve.emplace_back(thread(&Server::handlequethread, this));
        //thve.emplace_back(thread(&Server::gc, this));
        //thve[0].detach();
        //thve[1].detach();
        //thve[2].detach();

        //std::thread(&Server::fstart, this).detach();
        //FileServer fs;

        executor = make_shared< ilovers::TaskExecutor>(4);
        executor->commit(&Server::tmphandlethread, this);
        executor->commit(&Server::handlequethread, this);
        executor->commit(&Server::handle_login_write, this);
        executor->commit(&Server::gc, this);
        cout << "ASYNC SERVER START" << endl;
        service.run();
        cout << "server exit" << endl;
    }
    

    void clint_handle_accept(boost::system::error_code er, shared_ptr<clint>cl1);
    void gc();
    //开一个公共数据区开一个线程不断地读这个公共数据区如果该公共数据区有数据拿出来发送给相应的clint的消息接收容器
    //不对，如果是异步编程应该是去调用相应clintid的on_write方法
    char ttmphead[sizeof(Head)];
    Head* h;
    void changestatus(string& p, unsigned int st);
    static Head getHead(string& buff) {
        Head hh;
        memcpy(&hh, buff.c_str(),sizehead);
        return hh;
    }
public:
    //unordered_map<int, queue<string>>handleingque;
    //mutex handle_acc_mutex;
    void tmphandlethread();
    void handlequethread();
    void handle_login_write();
};

#endif