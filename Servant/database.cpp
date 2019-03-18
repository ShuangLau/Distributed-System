#include "database.h"


//std::unordered_map<std::string ,std::string> DataBase::fake_map ;
//std::unordered_map <std::string ,std::vector <std::string > > DataBase::fake_mail_map;
Connection DataBase::_conn =Configuration::connect("127.0.0.1:50051"); 
Connection Cookie::_conn =Configuration::connect("127.0.0.1:50051"); 


DataBase * DataBase::_instance = NULL;

DataBase ::DataBase()
{
    //fake_map["kevin"] = "123";
//    Table table = _conn.get_table("DataBase");
  //  std::string user_pass_row_key("user_pass");
    //Put put(user_pass_row_key);
    //put.add_column("user_password","kevin",(byte *)"123",3);
    //table.put(put);
}
DataBase * DataBase:: get_instance()
{
    if(_instance == NULL)
    {
	_instance = new DataBase();
    }
    return _instance;
}
int DataBase::update_password(const std::string & user_name ,const std::string & password)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("user_pass"));
    std::pair <byte * ,int > pair = result.get_value("user_password",user_name);
    if(pair.first != nullptr) {
	std::string user_pass_row_key("user_pass");
	Put put(user_pass_row_key);
	put.add_column("user_password",user_name,(byte *)password.c_str(),password.length());
	table.put(put);
	return OK;
    }
    else return NoSuchUser;

    /*
    if(fake_map.find(user_name) == fake_map.end())
    {
	return NoSuchUser;
    }
    fake_map[user_name] = password;
    return OK;
    */
}


int DataBase::check_user(const std::string & user_name ,const std::string & password)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("user_pass"));
    std::pair <byte * ,int > pair = result.get_value("user_password",user_name);
    if(pair.first == nullptr) return NoSuchUser;
    std::string get_password((char*)pair.first , pair.second);   
    std::cout <<"get paswd:"<<get_password<<":****"<<std::endl;
    std::cout <<"password :"<<password <<":*****"<<std::endl;
    std::cout << (password == get_password)<<std::endl;
    std::cout << (password.compare( get_password))<<std::endl;


    if(password == get_password) return OK;
    else return PassWordNotMatch;
    /*
    if(fake_map.find(user_name) == fake_map.end())
    {
	return NoSuchUser;
    }else if(fake_map[user_name] != password)
	return PassWordNotMatch;
    return OK;
    */
}

int DataBase:: register_user(const std::string & user_name ,const std::string &password)
{

    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("user_pass"));
    std::pair <byte * ,int > pair = result.get_value("user_password",user_name);
    if(pair.first == nullptr) {
	std::string user_pass_row_key("user_pass");
	Put put(user_pass_row_key);
	put.add_column("user_password",user_name,(byte *)password.c_str(),password.length() );
	table.put(put);
	return OK;
    }
    else return ExsitingUser;
    /*
    if(fake_map.find(user_name) == fake_map.end())
    {
	fake_map[user_name] = password;
	return OK;
    }else return ExsitingUser;
    */

}
bool DataBase :: has_user(const std::string & user_name)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("user_pass"));
    std::pair <byte * ,int > pair = result.get_value("user_password",user_name);
    if(pair.first == nullptr) return false;
    else return true;
    //return fake_map.find(user_name) != fake_map.end();
}

int DataBase :: append_mail(const std::string  &user_name ,char * content)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("mail_list"));
    std::pair <byte *,int > pair = result.get_value("user_mail_num",user_name);
    if(pair.first == nullptr)
    {
	// append number
	std::string row_key("mail_list");
	Put put_number(row_key);
	int one = 1;
	put_number.add_column("user_mail_num",user_name,(byte *) &one,sizeof(one));
	table.put(put_number);
	//append content
	
	Put put_content("mail_list");
	put_content.add_column(user_name+"_content",std::to_string(0),(byte*)content,strlen(content));
	table.put(put_content);
	return OK;
    }
    else {
	int user_mail_num = *(int *)( pair.first);
	std::string row_key("mail_list");
	Put put_number(row_key);
	user_mail_num++;
	put_number.add_column("user_mail_num",user_name,(byte *) &user_mail_num,sizeof(user_mail_num));
	table.put(put_number);
	//append content
	
	Put put_content("mail_list");
	put_content.add_column(user_name+"_content",std::to_string(user_mail_num-1),(byte*)content,strlen(content));
	table.put(put_content);
	return OK;
    }

    //fake_mail_map[user_name] .push_back(content);
    return OK;
}
int DataBase::delete_mail(const std::string & user_name, const std::string & mail_id)
{

    Table table = _conn.get_table("DataBase");
    //Result result = table.get(Get("mail_list"));
 //   std::pair <byte *,int > pair = result.get_value("user_mail_num",user_name);
  //  if(pair.first == nullptr)
   // {
//	return GetMailError;
 //   }
 //   else {
	//int user_mail_num = *(int *)( pair.first);
	//std::string row_key("mail_list");
	//Put put_number(row_key);
	//user_mail_num++;
	//put_number.add_column("user_mail_num",user_name,(byte *) &user_mail_num,sizeof(user_mail_num));
	//table.put(put_number);
	//append content
	
	Delete delete_content("mail_list");
	delete_content.add_column(user_name+"_content",mail_id);
	table.del(delete_content);
	return OK;
   // }

    //fake_mail_map[user_name] .push_back(content);
    return OK;

}
std::vector <std::string > DataBase ::get_mail(const std::string & user_name)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("mail_list"));
    std::pair <byte*,int > pair = result.get_value("user_mail_num",user_name);
    if( pair.first == nullptr) return std::vector <std::string>();
    else {
	std::vector <std::string> ans;
	int user_mail_num = *(int *)(pair.first);
	//Result content = table.get(Get(user_name+"_content"));
	for(int i = 0;i<user_mail_num;++i)
	{
	    std::pair <byte  *,int > content_pair = result.get_value(user_name+"_content",std::to_string(i));
	    if(content_pair.first == nullptr)
	    {
		std::cout <<"get mail error"<<std::endl;
		continue;
		//return std::vector<std::string>();
	    }
	    // store mail id
	    ans.push_back(std::to_string(i)+"\n"+std::string((char *)content_pair.first,content_pair.second));
	}
	return ans;
    }

    //std::pair <byte * ,int > 
    //if(fake_mail_map.find(user_name) == fake_mail_map.end())
//	return std::vector <std::string >();
    //return fake_mail_map[user_name];
}

void DataBase ::upload_file(const std::string & user_name){
    //StorageServiceUser user(user_name,_conn);
}














Cookie * Cookie::_instance = NULL;

//std::unordered_map <std::size_t , std::pair <std::string ,std::string > > Cookie::sid_map;
Cookie * Cookie::get_instance()
{
    if(_instance == NULL)
	_instance = new Cookie();
    return _instance;
}
Cookie::Cookie()
{
}
std::string Cookie::get_sid (const std::string &user_name,const std::string & password )
{

    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("Session"));
    std::hash<std::string > h_string;
    size_t session = h_string(user_name)+h_string(password);
    std::pair <byte *,int > pair = result.get_value("user_session",std::to_string(session));
    if(pair.first == nullptr)
    {
	Put put_session("Session");
	put_session.add_column("user_session",std::to_string(session),
		(byte *)user_name.c_str(),user_name.length());
	table.put(put_session);
    }
    return std::to_string(session);

     //std::hash<std::string> h;
     //if( sid_map .find(h(user_name +password)) == sid_map.end())
     //{
//	 sid_map[h(user_name +password)] = std::make_pair(user_name,password);
     //}
     //return h(user_name+password);
}

std::string Cookie::get_user_info_by_sid(const std::string & sid)
{
    Table table = _conn.get_table("DataBase");
    Result result = table.get(Get("Session"));
    std::pair <byte *, int > pair = result.get_value("user_session",sid);
    if(pair.first == nullptr )
	return std::string("");
    return std::string((char*)pair.first,pair.second);
    //if(sid_map.find(sid) == sid_map.end())
//	return std::make_pair("","");
    //else return sid_map[sid];
}

