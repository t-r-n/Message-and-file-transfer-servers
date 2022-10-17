#ifndef CON_MANAGER_H
#define CON_MANAGER_H
#include"server_config.h"
#include<set>
#include"FileClint.h"

class FileClint;
typedef std::shared_ptr<FileClint> FileClint_ptr;
class con_manager {
public:

	std::mutex cl_mutex;
	std::condition_variable cl_mutex_cond;
	std::atomic<bool>is_end;
	//io_context& service_;
	con_manager(const con_manager&) = delete;
	con_manager& operator=(const con_manager&) = delete;

	con_manager(){
		cout << "°ó¶¨gc" << endl;
		//service_.post(std::bind(&con_manager::gc, this));
		std::thread(&con_manager::gc, this).detach();

	}
	void start(FileClint_ptr ptr);
	void stop();
	void gc();


private:
	std::set<FileClint_ptr>cl;
};


#endif