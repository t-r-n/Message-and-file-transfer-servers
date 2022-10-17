#include"FileServer.h"
void FileServer::do_accept() {
    acceptor->async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
        {
            // Check whether the server was stopped by a signal before this
            // completion handler had a chance to run.
            if (!acceptor->is_open())
            {
                return;
            }

            if (!ec)
            {
                cout << "用户接入" << endl;
                con_manager_.start(std::make_shared<FileClint>(  //把链接（其实就是套接字）加入到容器中
                    std::move(socket), con_manager_));//同时这是connection的构造函数
            }

            do_accept();
        });
}
