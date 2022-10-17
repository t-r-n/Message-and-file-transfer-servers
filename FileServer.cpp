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
                cout << "�û�����" << endl;
                con_manager_.start(std::make_shared<FileClint>(  //�����ӣ���ʵ�����׽��֣����뵽������
                    std::move(socket), con_manager_));//ͬʱ����connection�Ĺ��캯��
            }

            do_accept();
        });
}
