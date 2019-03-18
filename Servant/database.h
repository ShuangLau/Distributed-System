#ifndef DATABASE_H
#define DATABASE_H
#include <unordered_map>
#include <string.h>
#include <vector>
#include "../storage-system/client/configuration.h"
#include "../storage-system/client/table.h"
#include "../storage-system/client/column-family-descriptor.h"
#include "../storage-system/client/result.h"
#include "../storage-service/storage-service-user.h"


class DataBase 
{
    public:
	enum ErrorCode {
	    NoSuchUser,
	    PassWordNotMatch,
	    ExsitingUser,
	    GetMailError,
	    OK
	};
	static DataBase * get_instance();
	static int check_user(const std::string & user_name, const std::string & password );
	static int register_user(const std::string & user_name ,const std::string & password);
	static bool has_user(const std::string & user_name);
	int update_password(const std::string & user_name ,const std::string & password);
	int append_mail(const std::string & user_name , char * content);
	int delete_mail(const std::string & user_name, const std::string & mail_id);
	void upload_file(const std::string & user_name);
	std::vector <std::string > get_mail(const std::string & user_name);

    private:
	DataBase();
	//static std::unordered_map <std::string,std::string > fake_map;
	//static std::unordered_map <std::string ,std::vector <std::string > > fake_mail_map;
	static DataBase * _instance;
	static Connection _conn;
	//static Table _table;

};
class Cookie
{
    public :
	static Cookie * get_instance();
	static std::string  get_sid(const std::string & user_name,const std::string & password );
	static std::string  get_user_info_by_sid(const std::string & sid); 

    private:
	Cookie();
	//static std::unordered_map <std::size_t , std::pair <std::string ,std::string > > sid_map;
	static Cookie *_instance;
	static Connection _conn;

};


#endif
