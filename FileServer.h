#ifndef FILESERVER_H
#define FILESERVER_H
#include"server_config.h"
#include"con_manager.h"
//#include"server.h"
class FileServer {
public:
	FileServer() {
        sock_ptr sock1(new ip::tcp::socket(service));
        sock = sock1;
        work_ptr = make_shared<boost::asio::io_context::work>(service);//����run�˳�
        sock->open(ip::tcp::v4());
        acceptor = shared_ptr< ip::tcp::acceptor>(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8007)));

        thread th(&FileServer::server_start, this);
        th.detach();


	}

    void server_start() {
        auto work(boost::asio::make_work_guard(service));
        do_accept();
        cout << "FileServer start" << endl;
        service.run();
        cout << "server exit" << endl;
    }
    void do_accept();



private:

    io_context service;
    shared_ptr<boost::asio::io_context::work> work_ptr;
    typedef shared_ptr<ip::tcp::socket> sock_ptr;
    typedef boost::system::error_code error_code;
    sock_ptr sock;
    shared_ptr< ip::tcp::acceptor>acceptor;
    //con_manager con_manager_=con_manager(service);
    con_manager con_manager_;//��ɾ������������࣬����ʱͨ����ʼ���б�����вι��캯����
                                                    //ֱ�ӵ��ûᱻ��������Ϊ����������
    shared_ptr< ilovers::TaskExecutor>executor;
    

};



#endif

