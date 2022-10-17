#include"FileClint.h"
#include<ctime>
#include<cstdlib>
#include"server_config.h"
FileClint::FileClint(ip::tcp::socket socket,con_manager& con) :socket_(std::move(socket)), con_manager_(con){

	is_delete.store(false);
	std::string sClientIp = socket_.remote_endpoint().address().to_string();
	unsigned short uiClientPort = socket_.remote_endpoint().port();
	cout << "对方ip地址" << sClientIp << "对方端口" << uiClientPort << endl;
}

void FileClint::do_read() {
	headbuf.clear();
	headbuf.resize(1024);
	async_read(socket_, buffer(headbuf), transfer_exactly(sizeofFFhead), [this](boost::system::error_code er, size_t sz) {
		//async_read(socket_, buffer(headbuf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
		if (er) {
			socket_.close();
			delete_me();
			return;
		}
		else {
			cout << "接收到的字节数" << sz << endl;
			Fhead* h = (Fhead*)headbuf.c_str();
			if (h->type == 'r') {
				try {
					sendto = h->sendto;
					acc = h->acc;
					content_length = h->content_length;
					filename = string(h->filename);
					//sizeoffilename = h->sizeoffilename;
					cout << h->acc << " " << h->sendto << " " << h->content_length << " " << h->filename << endl;
				}
				catch (...) {
					socket_.close();
					delete_me();
					return;
				}
				if (sendto > 0 && acc > 0 && content_length > 0 && filename.size() > 0) {
					async_write(socket_, buffer("ok"), [this](boost::system::error_code er, size_t sz) {
						if (er) {
							socket_.close();
							return;
						}
						else {
							cout << "已发送ok" << endl;
							do_content_read();
						}
						});
				}
			}
			else if (h->type == 's') {
				string path = "./" + string(h->filename);
				infile.open(path, std::ios::in | std::ios::binary);
				if (!infile.is_open()) {
					socket_.close();
					cout << "file is open error" << endl;
					delete_me();
					return;
				}
				try {
					std::array<char, BUFSIZE>buf;
					//char* buf = (char*)malloc(BUFSIZE);
					int len = 0;
					contentbuf.resize(INT_MAX / 2);
					contentbuf.clear();//记得加这句，把buffer清一清
					while (1) {
						len = infile.read(buf.data(), BUFSIZE).gcount();
						//len = infile.read(buf, BUFSIZE).gcount();
						if (len <= 0)break;
						contentbuf.append(string(buf.data(), len));
						//contentbuf.append(string(buf, len));
					}
					//free(buf);
				}catch (std::exception e) {
					cout << e.what();
					socket_.close();
					infile.close();
					delete_me();
					return;
				}
				//cout << "contlength" << contentbuf.size() << endl;
				infile.close();
				Fhead fh;
				sendto = h->sendto;
				acc = h->acc;
				filename = string(h->filename);
				fh.type = 's';
				fh.sendto = h->sendto;//填写文件发送者告知对方接收方已接收文件 ，到这说明接收方已经接收该文件
				fh.acc = h->acc;//接收方
				fh.content_length = contentbuf.size();
				strncpy_s(fh.filename, h->filename, strlen(h->filename));
				headbuf=string((char*)&fh, sizeofFFhead);
				cout << ((Fhead*)(headbuf.c_str()))->content_length << endl;
				async_write(socket_, buffer(headbuf), [this](boost::system::error_code er, size_t sz) {
					if (er) {
						socket_.close();
						delete_me();
						return;
					}
					else {
						cout << "已发送文件头" << endl;

						do_content_write();
					}
					});
			}

		}
		});
}
void FileClint::do_content_write() {
	async_write(socket_, buffer(contentbuf, contentbuf.size()), [this](boost::system::error_code er, std::size_t) {
		if (er) {
			cout << er.what() << endl;
			socket_.close();
			delete_me();
			return;
		}
		else {
			std::string().swap(contentbuf);

			socket_.close();
			//cout << contentbuf.capacity();
			cout << "发送成功" << endl;
			delete_me();
			

			//告诉发送者对方已接收
			Head hd;
			hd.account = this->acc;
			hd.sendto = this->sendto;
			hd.type = 's';
			hd.length = filename.size();
			hd.status = 2;
			string se((char*)&hd, sizehead);
			cout << "发送给" << hd.sendto << "发送者：" << hd.account << endl;
			se.append(filename);
			{
				std::lock_guard<mutex>sbguard(handle_acc_mutex);
				handleingque[sendto].push(se);
			}
			is_have_task1 = true;
			semu_cond1.notify_all();

		}
		});
}
void FileClint::do_content_read() {
	if (init) {
		//打开文件描述符
		char tmp[1024];
		unsigned seed;  // Random generator seed
	// Use the time function to get a "seed” value for srand
		seed = time(0);
		srand(seed);
		// Now generate and print three random numbers
		//cout << rand() << " ";
		snprintf(tmp, sizeof(tmp),"./F_%d", rand()%10000);
		filename.insert(0,string(tmp));
		outfile.open(filename,ios::out|ios::binary|ios::trunc);
		init = false;
		if (outfile.is_open()) {
			cout << "初始化结束" << endl;
		}
		else {
			socket_.close();
			delete_me();
		}
		starttime = std::chrono::system_clock::now();
		//contentbuf.resize(65536);//放着还不行必须每次重新指定大小
	}
	//contentbuf.clear();
	contentbuf.resize(65536);
	async_read(socket_, buffer(contentbuf), transfer_at_least(1), [this](boost::system::error_code er, size_t sz) {
		if (er) {
			socket_.close();
			delete_me();
			return;
		}
		else {
			iocount++;
			//cout << "进行一次异步读取任务" << endl;
			if (outfile.is_open()) {
				outfile.write(contentbuf.c_str(), sz);
			//获取字节数计算content位置
				if (outfile.tellp() == content_length) {//获取当前文件流的位置
					//cout << "结束" << endl;
					auto endtime = std::chrono::system_clock::now();
					try {
						auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endtime - starttime);

						cout << "下载速度为" << ((double)(((double)content_length) / 1024.0 / 1024.0)) / (((double)duration.count()) / 1000.0 / 1000.0) << "mb/s" << endl;
						cout << "进行了" << iocount << "次tcpio" << endl;
						//通知对方接收文件
						//auto times = endtime - starttime;
						//cout << times.count() << endl;

					}
					catch (std::exception e) {
						cout << e.what() << endl;
					}
					outfile.close();
					socket_.close();



					//通知接收者接收文件
					Head hd;
					hd.account =  this->acc;
					hd.sendto = this->sendto;
					cout << "要发送给" << hd.sendto << "发送者是：" << acc << endl;
					hd.type = 'q';
					hd.length = filename.size();
					hd.status = 1;
					string se((char*)&hd, sizehead);
					se.append(filename);
					{
						std::lock_guard<mutex>sbguard(handle_acc_mutex);
						handleingque[sendto].push(se);
					}
					is_have_task1 = true;
					semu_cond1.notify_all();



					delete_me();
					return;
				}
				else if (outfile.tellp() > content_length) {
					socket_.close();
					delete_me();
					return;
				}
				//cout << "当前流的大小" << outfile.tellp() << endl;
				//cout << "一次读入了几个字节" << sz << endl;
				
			}
			do_content_read();
		}
		});
}
void FileClint::delete_me() {
	is_delete.store(true);
	con_manager_.is_end.store(true);//原子的开始通知gc机制可以销毁对象了；
	con_manager_.cl_mutex_cond.notify_all();
}