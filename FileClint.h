#ifndef FILECLINT_H
#define FILECLINT_H
#include"server_config.h"
#include"con_manager.h"
struct Fhead {
	char type;  //r,收文件，s发文件
	int sendto;
	int content_length;
	int acc;
	char filename[256];
	//int sizeoffilename;

};
const int sizeofFFhead(sizeof(Fhead));
const int BUFSIZE = 40960;
#include<fstream>

class con_manager;
class FileClint {
	int iocount = 0;
	ip::tcp::socket socket_;
	con_manager &con_manager_;
	int acc;
	int sendto;//0表示发送给服务器
	int content_length;
	string filename;
	int sizeoffilename = 0;
	string headbuf;
	string contentbuf;
	bool init = true;
	ofstream outfile;
	ifstream infile;

	//下载速度记录部分
	decltype(std::chrono::system_clock::now()) starttime; //auto在局部变量声明的时候用，这个在未初始化但需要auto类似的功能时用


public:
	std::atomic<bool> is_delete;
	FileClint(const FileClint&) = delete;
	FileClint& operator=(const FileClint&) = delete;
	explicit FileClint(ip::tcp::socket socket,con_manager& con);
	void do_read();
	void do_content_read();
	void do_content_write();
	void delete_me();

};

#endif
