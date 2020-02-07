/*
 * classes.h
 *
 *  Created on: Jan 24, 2020
 *      Author: dad
 */

#ifndef CLASSES_H_
#define CLASSES_H_

#include <string>
#include <vector>
#ifdef WINDOZE
#else
#include <future>
#include <curl/curl.h>
#endif

using std::string;
using std::vector;
#ifdef WINDOZE
#else
using std::future;
#endif



class Downloadcacapi
{
public:
	string download(const string& id);
	string list_files();
	vector<string> download_multi(const vector<string> codes);
	string upload(const string& data, const string& savename);
//	struct Upload_multi_arg
//	{
//		string data;
//		string savename;
//	};
//	vector<string> upload_multi(
//			const vector<Downloadcacapi::Upload_multi_arg> args);
	void delete_upload(const string& id);
//	future<bool> async_delete_upload(const string& id);
	Downloadcacapi(const string& username, const string& password);
	~Downloadcacapi();
	vector<string> get_cookies();
private:
//	time_t last_cookie_refresh_time; // todo..
	//todo: refreshcookiesession()
	const string username;
	const string password;
	string curl_exec(const string& url);
	void curl_exec_faster1(const string& url);
	CURL *ch;
	string responsebuf;
	void login();
	void logout();
	std::thread cookie_session_keepalive_thread;
	CURL *cookie_session_keepalive_thread_curl;
	time_t last_interaction_time;
	static size_t curl_write_hook(const void * read_ptr, const size_t size,
			const size_t count, void *s_ptr);

};





#endif /* CLASSES_H_ */
