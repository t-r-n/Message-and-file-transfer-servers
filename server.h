//#pragma once
#ifndef SERVER_H
#define SERVER_H
#include "server_config.h"
#include"clint.h"
#include"FileServer.h"
class Server :public std::enable_shared_from_this<Server> {//������Ӧ�ÿ�һ���̳߳ض�ȡ�ͻ�����Ϣ���е�����Ӧ�ͻ���on_write����
public:
    string buf;
    io_service service;
    typedef shared_ptr<ip::tcp::socket> sock_ptr;
    typedef boost::system::error_code error_code;
    sock_ptr sock;
    shared_ptr< ip::tcp::acceptor>acceptor;
    int curdfileid = 0; //���������ǰ�ӽ��������ļ�Ӧ�ø����id
    int curclintid = 0;
    list<shared_ptr<clint>>cl;
    shared_ptr< ilovers::TaskExecutor>executor;
    Server() {
        sock_ptr sock1(new ip::tcp::socket(service));
        sock = sock1;
        boost::asio::io_service::work work(service);//����run�˳�
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
    //��һ��������������һ���̲߳��ϵض������������������ù����������������ó������͸���Ӧ��clint����Ϣ��������
    //���ԣ�������첽���Ӧ����ȥ������Ӧclintid��on_write����
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