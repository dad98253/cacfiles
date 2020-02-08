/*
 * cfcfiles.cpp
 *
 *  Created on: Jan 21, 2020
 *      Author: dad
 */

#include <iostream>

#ifdef WINDOZE
#include <stdlib.h>
#include <cstddef>
#include "..\curl-7.68.0\include\curl\curl.h"
#else
#include <error.h>
#include <err.h>
#include <execinfo.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>
#include <shared_mutex>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <atomic>
#include <inttypes.h> // todo: proper printf macros
#include <unordered_map>
#include <linux/nbd.h>
#include <curl/curl.h>
#include <errno.h>
#endif
#include <assert.h>
#include <string>
#include <cstring>
#include <functional>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <vector>
#include <math.h>
#include <algorithm> // std::all_of

#include "classes.h"

//using namespace std;
#ifdef WINDOZE
#define __BYTE_ORDER __LITTLE_ENDIAN
#else
using std::atomic_uint;
using std::mutex;
//using std::this_thread;
using std::shared_mutex;
using std::to_string;
using std::thread;
using std::future;
using std::promise;
using std::atomic_bool;
using std::array;
#endif
using std::cout;
using std::endl;
using std::string;
using std::runtime_error;
using std::vector;
using std::flush;
using std::cerr;


extern string argv_username;
extern string argv_password;

enum
{
	SECTOR_SIZE = 4096, BLOCK_SIZE = 4096, CAC_CODE_SIZE = 25
};
#ifdef WINDOZE
#define to_string CString
#else
static_assert((sizeof(size_t) > 4),"32bit systems are not officially supported.. feel free to complain in the bugtracker if this is an issue for you. (sizeof(size_t) <=4)" );
#endif

FILE *close_me_on_cleanup = NULL;
// <headache>
void install_shutdown_signal_handlers(void);
void exit_global_cleanup(void);
//</headache>
#if !defined(likely)
// compilers known to support it: gnu gcc, intel icc, clang, IBM C, Cray C
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__clang__) ||    \
    (defined(__IBMC__) || defined(__IBMCPP__)) || defined(_CRAYC)
#if defined(__cplusplus)
// https://stackoverflow.com/a/43870188/1067003
#define likely(x) __builtin_expect(static_cast<bool>((x)), 1)
#define unlikely(x) __builtin_expect(static_cast<bool>((x)), 0)
#else  // __cplusplus
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif  // __cplusplus
#else  // __GNUC__ ...
// compilers known to _not_ support it: tcc (TinyC), msvc, Digital Mars
#if defined(__TINYC__) || defined(_MSC_VER) || defined(__DMC__)
#define likely(x) (x)
#define unlikely(x) (x)
#else  //  __TINYC__ ...
// unknown compiler
#warning support for __builtin_expect() is unknown on this compiler. please submit a bugreport.
#define likely(x) (x)
#define unlikely(x) (x)
#endif  //  __TINYC__ ...
#endif   // __GNUC__ ...
#endif  // !defined(likely)

#if !defined(UNREACHABLE)
//TODO: check MSVC/ICC
#if defined(__GNUC__) || defined(__clang__)
#define UNREACHABLE() (__builtin_unreachable())
#else  // __GNUC__ ...
//not sure what to do here...
#define UNREACHABLE() ()
#endif  // __GNUC__ ...
#endif  // !defined(UNREACHABLE)

#if !defined(NTOHL)
#if !defined(__BYTE_ORDER)
#error Failed to detect byte order! fix the code yourself and/or submit a bugreport!
#endif  // !defined(__BYTE_ORDER)
#if __BYTE_ORDER == __BIG_ENDIAN
/* The host byte order is the same as network byte order,
 so these functions are all just identity.  */
#define NTOHL(x)	((uint32_t)x)
#define NTOHS(x)	((uint16_t)x)
#define HTONL(x)	((uint32_t)x)
#define HTONS(x)	((uint16_t)x)
#define HTONLL(x)	((uint64_t)x)
#define NTOHLL(x)	((uint64_t)x)
#else  // __BYTE_ORDER == __BIG_ENDIAN
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define NTOHL(x)	__bswap_constant_32 ((uint32_t)x)
#define NTOHS(x)	__bswap_constant_16 ((uint16_t)x)
#define HTONL(x)	__bswap_constant_32 ((uint32_t)x)
#define HTONS(x)	__bswap_constant_16 ((uint16_t)x)
#define HTONLL(x)	__bswap_constant_64 ((uint64_t)x)
#define NTOHLL(x)	__bswap_constant_64 ((uint64_t)x)
#else  // __BYTE_ORDER == __LITTLE_ENDIAN
#error Failed to detect byte order! fix the code yourself and/or submit a bugreport!
#endif // __BYTE_ORDER == __LITTLE_ENDIAN
#endif // __BYTE_ORDER == __BIG_ENDIAN
#endif // !defined(NTOHL)

#ifdef WINDOZE
#define macrobacktrace() ()
int myerror(int status,int errnum,...)
{	
	va_list args;
	va_start(args, errnum);
//	error_at_line(status,errnum,__FILE__,__LINE__,__VA_ARGS__);
	va_end(args);
	return (0);
}
#else
#define macrobacktrace() { \
void *array[20]; \
int traces=backtrace(array,sizeof(array)/sizeof(array[0])); \
if(traces<=0) { \
	fprintf(stderr,"failed to get a backtrace!"); \
} else { \
backtrace_symbols_fd(array,traces,STDERR_FILENO); \
} \
}

#define myerror(status,errnum,...){macrobacktrace();error_at_line(status,errnum,__FILE__,__LINE__,__VA_ARGS__);}
#endif
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3451.pdf

template<typename T>

void *emalloc(const size_t size)
{
	void *ret = malloc(size);
	if (unlikely(size && !ret))
	{
		myerror(EXIT_FAILURE, errno,
				"malloc failed to allocate %zu bytes. terminating...\n", size);
	}
	return ret;
}
void *erealloc(void *ptr, const size_t size)
{
	void *ret = realloc(ptr, size);
	if (unlikely(size && !ret))
	{
		myerror(EXIT_FAILURE, errno, "realloc failed to allocate %zu bytes.",
				size);
	}
	return ret;
}
void *ecalloc(const size_t num, const size_t size)
{
	void *ret = calloc(num, size);
	if (unlikely(num > 0 && size > 0 && !ret))
	{
		myerror(EXIT_FAILURE, errno, "calloc failed to allocate %zu*%zu bytes.",
				num, size);
	}
	return ret;
}
int efseek(FILE *stream, const long int offset, const int origin)
{
	const int ret = fseek(stream, offset, origin);
	if (unlikely(ret != 0))
	{
		myerror(EXIT_FAILURE, errno, "fseek() failed to seek to %lu", offset);
	}
	return ret;
}
size_t efwrite(const void * ptr, const size_t size, const size_t count,
		FILE * stream)
{
	const size_t ret = fwrite(ptr, size, count, stream);
	if (unlikely(size > 0 && count > 0 && ret != count))
	{
		myerror(EXIT_FAILURE, ferror(stream),
				"fwrite() failed to write %lu bytes! errno: %i ferror: %i",
				(size * count), errno, ferror(stream));
	}
	return ret;
}
#ifdef WINDOZE
#else
static_assert(CURL_AT_LEAST_VERSION(7,56,0),"this program requires libcurl version >= \"7.56.0\", your libcurl version is \"" LIBCURL_VERSION "\", which is too old." );
#endif
//this  almost has to be a macro because of how curl_easy_setopt is made (a macro taking different kinds of parameter types)

#ifdef WINDOZE
#define ecurl_easy_setopt(handle,opt,param) curl_easy_setopt(handle,opt,param)
#else
#define ecurl_easy_setopt(handle, option, parameter)({ \
CURLcode ret_8uyr7t6sdygfhd=curl_easy_setopt(handle,option,parameter); \
if(unlikely( ret_8uyr7t6sdygfhd  != CURLE_OK)){ \
	 myerror(EXIT_FAILURE,errno,"curl_easy_setopt failed to set option %i. CURLcode: %i curl_easy_strerror: %s\n", option, ret_8uyr7t6sdygfhd, curl_easy_strerror(ret_8uyr7t6sdygfhd));   \
	} \
});
#endif
#define ecurl_multi_setopt(multi_handle,option, parameter)({ \
		CURLMcode ret_7yruhj=curl_multi_setopt(multi_handle,option,parameter);\
		if(unlikely(ret_7yruhj!= CURLM_OK)){ \
			myerror(EXIT_FAILURE,errno,"curl_multi_setopt failed to set option %i CURLMcode: %i curl_multi_strerror: %s\n",option,ret_7yruhj,curl_multi_strerror(ret_7yruhj));\
		} \
});

#define ecurl_multi_add_handle(multi_handle, easy_handle)({ \
	CURLMcode ret_8ujih=curl_multi_add_handle(multi_handle,easy_handle);\
	if(unlikely(ret_8ujih!=CURLM_OK)){ \
		myerror(EXIT_FAILURE,errno,"curl_multi_add_handle failed. CURLMcode: %i curl_multi_strerror: %s\n",ret_8ujih,curl_multi_strerror(ret_8ujih)); \
	}\
});
#ifdef WINDOZE
int ecurl_easy_getinfo(void * curl,CURLINFO info,...)
{	
	va_list args;
	va_start(args, info);
	CURLcode ret=curl_easy_getinfo(curl,info,args); 
	if(unlikely(ret!=CURLE_OK)){ 
		 myerror(EXIT_FAILURE,errno,"curl_easy_getinfo failed to get info %i. CURLCode: %i curl_easy_strerror: %s\n", info, ret, curl_easy_strerror(ret));
	} 
	va_end(args);
	return (0);
}
#else
#define ecurl_easy_getinfo(curl,info,...){ \
	CURLcode ret=curl_easy_getinfo(curl,info,__VA_ARGS__); \
	if(unlikely(ret!=CURLE_OK)){ \
		 myerror(EXIT_FAILURE,errno,"curl_easy_getinfo failed to get info %i. CURLCode: %i curl_easy_strerror: %s\n", info, ret, curl_easy_strerror(ret));   \
	} \
}
#endif
std::string curlprettyerror(const string name, const CURLcode errnum)
{
#ifdef WINDOZE
	char buffer [33];
	return string(
			name + " error " + itoa(errnum,buffer,10) + ": "
#else
	return string(
			name + " error " + to_string(errnum) + ": "
#endif
					+ string(curl_easy_strerror(errnum)));
}
CURL* ecurl_easy_init()
{
	CURL *ret = curl_easy_init();
	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno,
				"curl_easy_init() failed! (why? i have no idea.)");
	}
	return ret;
}
CURLM* ecurl_multi_init()
{
	CURLM *ret = curl_multi_init();
	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno, "curl_multi_init() failed!");
	}
	return ret;
}
curl_mime* ecurl_mime_init(CURL *easy_handle)
{
	curl_mime *ret = curl_mime_init(easy_handle);
	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno, "curl_mime_init() failed!");
	}
	return ret;
}
curl_mimepart* ecurl_mime_addpart(curl_mime *mime)
{
	curl_mimepart *ret = curl_mime_addpart(mime);
	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno, "curl_mime_addpart() failed!");
	}
	return ret;

}
struct curl_slist* ecurl_slist_append(struct curl_slist * list,
		const char * c_string)
{
	struct curl_slist *ret = curl_slist_append(list, c_string);
	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno, "curl_slist_append() failed!");
	}
	return ret;
}
CURLcode ecurl_mime_data(curl_mimepart * part, const char * data,
		size_t datasize)
{
	CURLcode ret = curl_mime_data(part, data, datasize);
	if (unlikely(ret != CURLE_OK))
	{
		throw runtime_error(curlprettyerror("ecurl_mime_data() failed!", ret));
	}
	return ret;
}
CURLcode ecurl_mime_name(curl_mimepart * part, const char * name)
{
	CURLcode ret = curl_mime_name(part, name);
	if (unlikely(ret != CURLE_OK))
	{
		throw runtime_error(curlprettyerror("curl_mime_name() failed!", ret));
	}
	return ret;
}
CURLcode ecurl_mime_filename(curl_mimepart * part, const char * filename)
{
	CURLcode ret = curl_mime_filename(part, filename);
	if (unlikely(ret != CURLE_OK))
	{
		throw runtime_error(
				curlprettyerror("curl_mime_filename() failed!", ret));
	}
	return ret;

}
CURLcode ecurl_easy_perform(CURL *easy_handle)
{
	CURLcode ret = curl_easy_perform(easy_handle);
	if (unlikely(ret != CURLE_OK))
	{
		throw runtime_error(curlprettyerror("curl_easy_perform()", ret));
	}
	return ret;
}

CURLMcode ecurl_multi_perform(CURLM *multi_handle, int *running_handles)
{
	CURLMcode ret = curl_multi_perform(multi_handle, running_handles);
	if (unlikely(ret != CURLM_OK))
	{
		myerror(EXIT_FAILURE, errno,
				"curl_multi_perform failed. CURLMcode: %i curl_multi_strerror: %s\n",
				ret, curl_multi_strerror(ret));
	}
	return ret;
}
CURLMcode ecurl_multi_wait(CURLM *multi_handle, struct curl_waitfd extra_fds[],
		unsigned int extra_nfds, int timeout_ms, int *numfds)
{

	CURLMcode ret = curl_multi_wait(multi_handle, extra_fds, extra_nfds,
			timeout_ms, numfds);
	if (unlikely(ret != CURLM_OK))
	{
		myerror(EXIT_FAILURE, errno,
				"curl_multi_wait failed. CURLMcode: %i curl_multi_strerror: %s\n",
				ret, curl_multi_strerror(ret));
	}
	return ret;
}
CURLMcode ecurl_multi_cleanup(CURLM *multi_handle)
{
	CURLMcode ret = curl_multi_cleanup(multi_handle);
	if (unlikely(ret != CURLM_OK))
	{
		myerror(EXIT_FAILURE, errno,
				"curl_multi_cleanup failed. CURLMcode: %i curl_multi_strerror: %s\n",
				ret, curl_multi_strerror(ret));

	}
	return ret;
}
CURL *ecurl_easy_duphandle(CURL *handle)
{
	errno = 0;
	cout << "dup!" << endl;
	CURL *ret = curl_easy_duphandle(handle);

	if (unlikely(ret==NULL))
	{
		myerror(EXIT_FAILURE, errno, "curl_easy_duphandle() failed! ");
	}
	return ret;
}
CURL *ecurl_easy_duphandle_with_cookies(CURL *easy_handle)
{
	CURL *ret = ecurl_easy_duphandle(easy_handle);
	struct curl_slist *cookies = NULL;
	CURLcode res = curl_easy_getinfo(easy_handle, CURLINFO_COOKIELIST,
			&cookies);
	if (res == CURLE_OK && cookies)
	{
		struct curl_slist *each = cookies;
		while (each)
		{
			ecurl_easy_setopt(ret, CURLOPT_COOKIELIST, each->data);
			each = each->next;
		}
		curl_slist_free_all(cookies);
	}
	return ret;
}
void ecurl_clone_cookies(CURL *source_handle, CURL *target_handle)
{
	struct curl_slist *cookies = NULL;
	CURLcode res = curl_easy_getinfo(source_handle, CURLINFO_COOKIELIST,
			&cookies);
	if (res == CURLE_OK && cookies)
	{
		struct curl_slist *each = cookies;
		while (each)
		{
			ecurl_easy_setopt(target_handle, CURLOPT_COOKIELIST, each->data);
			each = each->next;
		}
		curl_slist_free_all(cookies);
	}

}
char *ecurl_easy_escape(CURL * curl, const char * instring, const int length)
{
	char *ret = curl_easy_escape(curl, instring, length);
	if (unlikely(ret==NULL))
	{
		throw runtime_error(
				string(
						"curl_easy_escape() failed! why? dunno, out of ram? input string: ")
						+ instring);
	}
	return ret;
}
string urlencode(const string& str)
{
	char *escaped = curl_escape(str.c_str(), str.length());
	if (unlikely(escaped==NULL))
	{
		throw runtime_error("curl_escape failed!");
	}
	string ret = escaped;
	curl_free(escaped);
	return ret;
}

size_t Downloadcacapi::curl_write_hook(const void * read_ptr, const size_t size,
		const size_t count, void *s_ptr)
{
	(*(string*) s_ptr).append((const char*) read_ptr, size * count);
	return count;
}

string Downloadcacapi::list_files()
{
	this->last_interaction_time = time(NULL);

	ecurl_easy_setopt(this->ch, CURLOPT_POST, 1);
//	ecurl_easy_setopt(this->ch, CURLOPT_COPYPOSTFIELDS,
//			string("act=1&filecode=" + urlencode(id)).c_str());
	string response = this->curl_exec(
			"https://download.cloudatcost.com/user/uploaded_files.php");
//    string response=this->curl_exec("http://dumpinput.ratma.net");
/*	if (unlikely(response.length() == 1 || response[0] != 'y'))
	{
		std::cerr << "response length: " << response.length() << ": "
				<< response << std::endl;
		throw std::runtime_error(
				"failed to delete upload! got invalid response from delete api.");
	} */
#ifdef DEBUG
	cerr << "files listed " << endl;
#endif
	response.erase(0, 2);
	return response;
}



string Downloadcacapi::download(const string& id)
{
	this->last_interaction_time = time(NULL);
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPGET, 1);
	return (this->curl_exec(
			"https://download.cloudatcost.com/user/download.php?filecode="
					+ urlencode(id)));
}

string Downloadcacapi::upload(const string& data, const string& savename)
{
	this->last_interaction_time = time(NULL);
	curl_mime *mime1 = NULL;
	curl_mimepart *part1 = NULL;
	struct curl_slist *slist1 = NULL;
	slist1 = ecurl_slist_append(slist1, "X-Requested-With: XMLHttpRequest");
	mime1 = ecurl_mime_init(this->ch);
	part1 = ecurl_mime_addpart(mime1);
	ecurl_mime_data(part1, "", CURL_ZERO_TERMINATED);
	ecurl_mime_name(part1, "days");
	part1 = ecurl_mime_addpart(mime1);
	ecurl_mime_data(part1, "", CURL_ZERO_TERMINATED);
	ecurl_mime_name(part1, "downloads");
	part1 = ecurl_mime_addpart(mime1);
	ecurl_mime_data(part1, "", CURL_ZERO_TERMINATED);
	ecurl_mime_name(part1, "password");
	part1 = ecurl_mime_addpart(mime1);
//curl_mime_filedata(part1, "/dev/null");
	ecurl_mime_data(part1, &data[0], data.length());
	ecurl_mime_filename(part1, savename.c_str());
	ecurl_mime_name(part1, "file");
	ecurl_easy_setopt(this->ch, CURLOPT_MIMEPOST, mime1);
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPHEADER, slist1);
//string response=this->curl_exec("http://dumpinput.ratma.net");
	string response = this->curl_exec(
			"https://download.cloudatcost.com/upload.php");
	ecurl_easy_setopt(this->ch, CURLOPT_MIMEPOST, NULL);
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPHEADER, NULL);
	curl_mime_free(mime1);
	mime1 = NULL; //
	curl_slist_free_all(slist1);
	slist1 = NULL;
	if (unlikely(response.length() != 27))
	{
		std::cerr << "response length: " << response.length() << ": "
				<< response << "\n";
		throw std::runtime_error(
				"upload response length was not 27 bytes long! something went wrong");
	}
	if (unlikely(response[0] != '1' || response[1] != '|'))
	{
		std::cerr << "(invalid) upload response: " << response << "\n";
		throw std::runtime_error("upload response was invalid!");
	}
	response.erase(0, 2);
	return response;
}
void Downloadcacapi::delete_upload(const string& id)
{
	this->last_interaction_time = time(NULL);

	ecurl_easy_setopt(this->ch, CURLOPT_POST, 1);
	ecurl_easy_setopt(this->ch, CURLOPT_COPYPOSTFIELDS,
			string("act=1&filecode=" + urlencode(id)).c_str());
	string response = this->curl_exec(
			"https://download.cloudatcost.com/user/uploaded_files.php");
//    string response=this->curl_exec("http://dumpinput.ratma.net");
	if (unlikely(response.length() != 1 || response[0] != 'y'))
	{
		std::cerr << "response length: " << response.length() << ": "
				<< response << std::endl;
		throw std::runtime_error(
				"failed to delete upload! got invalid response from delete api.");
	}
#ifdef DEBUG
	cerr << "deleted " << id << endl;
#endif
}

Downloadcacapi::Downloadcacapi(const string& username, const string& password) :
		username(username), password(password)
{
	this->ch = ecurl_easy_init();
	ecurl_easy_setopt(this->ch, CURLOPT_WRITEFUNCTION,
			Downloadcacapi::curl_write_hook);
	ecurl_easy_setopt(this->ch, CURLOPT_WRITEDATA, &this->responsebuf);
	ecurl_easy_setopt(this->ch, CURLOPT_AUTOREFERER, 1);
	ecurl_easy_setopt(this->ch, CURLOPT_FOLLOWLOCATION, 1);
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPGET, 1);
	ecurl_easy_setopt(this->ch, CURLOPT_SSL_VERIFYPEER, 0); // fixme?
	ecurl_easy_setopt(this->ch, CURLOPT_CONNECTTIMEOUT, 8);
	//ecurl_easy_setopt(this->ch, CURLOPT_TIMEOUT, 60); // how long to wait for 4096? dunno...
	ecurl_easy_setopt(this->ch, CURLOPT_COOKIEFILE, "");
	ecurl_easy_setopt(this->ch, CURLOPT_TCP_KEEPALIVE, 1L);
	ecurl_easy_setopt(this->ch, CURLOPT_ACCEPT_ENCODING, ""); // this should make login/logout faster, but it might make sector upload/download slower, or just use more cpu.. hmm
	ecurl_easy_setopt(this->ch, CURLOPT_USERAGENT, "cacdrive-dev");
	this->login();
	this->cookie_session_keepalive_thread_curl = ecurl_easy_init();
	ecurl_clone_cookies(this->ch, this->cookie_session_keepalive_thread_curl);
	this->last_interaction_time = time(NULL);
	cookie_session_keepalive_thread =
			std::thread(
					[this]()->void
					{
						string unused_reply_buffer;
						ecurl_easy_setopt(this->cookie_session_keepalive_thread_curl,CURLOPT_WRITEDATA,&unused_reply_buffer);
						ecurl_easy_setopt(this->cookie_session_keepalive_thread_curl,CURLOPT_URL,"https://download.cloudatcost.com/user/settings.php");
						ecurl_easy_setopt(this->cookie_session_keepalive_thread_curl,CURLOPT_HTTPGET,1);
						ecurl_easy_setopt(this->cookie_session_keepalive_thread_curl,CURLOPT_WRITEFUNCTION,Downloadcacapi::curl_write_hook);
						for(;;)
						{
							//this_thread::sleep_for(chrono::seconds(1));
							 sleep(1);  //  added to avoid use of this_thread
							if(this->last_interaction_time==0)
							{
								// 0 is a magic value for `time to shut down`.
								return;
							}
							if(this->last_interaction_time > (time(NULL)-(20*60)))
							{
								continue;
							}
							//cout << "20min ping.. last: " << this->last_interaction_time << " - now: " << time(NULL) << endl;
							//been over 10 minutes since the last interaction, time to send a keepalive ping.
							ecurl_easy_perform(this->cookie_session_keepalive_thread_curl);
							this->last_interaction_time=time(NULL);
							unused_reply_buffer.clear();
						}
					});
} //
Downloadcacapi::~Downloadcacapi()
{
	this->last_interaction_time = 0; // magic value for shut down.
	this->cookie_session_keepalive_thread.join(); // a signal would be faster/better... not sure how to implement that.
	this->logout();
	curl_easy_cleanup(this->ch);
	curl_easy_cleanup(this->cookie_session_keepalive_thread_curl);
}
void Downloadcacapi::login()
{
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPGET, 1);
	this->curl_exec_faster1("https://download.cloudatcost.com/user/login.php"); // just need a cookie session.
	ecurl_easy_setopt(this->ch, CURLOPT_POST, 1);
	ecurl_easy_setopt(this->ch, CURLOPT_COPYPOSTFIELDS,
			string(
					string("username=") + urlencode(this->username)
							+ string("&password=") + urlencode(this->password)
							+ "&submit=").c_str());
	string response = this->curl_exec(
			"https://download.cloudatcost.com/user/manage-check.php");
//    string response=this->curl_exec("dumpinput.ratma.net");
	if (response.find("LOGOUT") == string::npos)
	{
		std::cerr << "len: " << response.length() << " - " << response
				<< std::endl;
		throw runtime_error("failed to login - cannot find the logout button!");
	}
}
void Downloadcacapi::logout()
{
	ecurl_easy_setopt(this->ch, CURLOPT_HTTPGET, 1);
	this->curl_exec("https://download.cloudatcost.com/user/logout.php");
}
string Downloadcacapi::curl_exec(const string& url)
{
	this->responsebuf.clear();
	ecurl_easy_setopt(this->ch, CURLOPT_URL, url.c_str());
	ecurl_easy_perform(this->ch);
	return string(this->responsebuf);
}
void Downloadcacapi::curl_exec_faster1(const string& url)
{
	this->responsebuf.clear();
	ecurl_easy_setopt(this->ch, CURLOPT_URL, url.c_str());
	ecurl_easy_perform(this->ch);
	return;
}
vector<string> Downloadcacapi::get_cookies()
{
	vector<string> ret;
	struct curl_slist *cookies = NULL;
	CURLcode res = curl_easy_getinfo(this->ch, CURLINFO_COOKIELIST, &cookies);
	if (res == CURLE_OK && cookies)
	{
		struct curl_slist *each = cookies;
		while (each)
		{
			ret.push_back(string(string(each->data)));
			each = each->next;
		}
		curl_slist_free_all(cookies);
	}
	return ret;
}


void exit_global_cleanup(void)
{
	static mutex single_exit_global_cleanup_mutex;
	{
		if (unlikely(!single_exit_global_cleanup_mutex.try_lock()))
		{
			myerror(0, errno,
					"Warning: more than 1 thread tried to run exit_global_cleanup! thread id %zu prevented..\n",
					pthread_self());
			return;
		}
	}
	printf("shutting down, cleaning up.. thread doing the cleanup: %zu \n",
			pthread_self());

	if (close_me_on_cleanup)
	{
		fclose(close_me_on_cleanup);
	}
	curl_global_cleanup();
}
void shutdown_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	(void) context;
	myerror(EXIT_FAILURE, errno,
			"received shutdown signal %i (%s) from PID %i / UID %i. shutting down..\n",
			sig, strsignal(sig), (int) siginfo->si_pid, (int) siginfo->si_uid);

}
void install_shutdown_signal_handler(const int sig)
{
//yes, void. i terminate if there's an error.
	struct sigaction act =
	{ 0 };
	act.sa_sigaction = &shutdown_signal_handler;
	act.sa_flags = SA_SIGINFO;
	if (unlikely(-1==sigaction(sig, &act, NULL)))
	{
		myerror(EXIT_FAILURE, errno,
				"failed to install signal handler for %i (%s)\n", sig,
				strsignal(sig));
	}
}
void install_shutdown_signal_handlers(void)
{
#if defined(_POSIX_VERSION)
#if _POSIX_VERSION>=199009L
//daemon mode not supported (yet?)
	install_shutdown_signal_handler(SIGHUP);
	install_shutdown_signal_handler(SIGINT);
	install_shutdown_signal_handler(SIGQUIT);
	install_shutdown_signal_handler(SIGILL);		//?
	install_shutdown_signal_handler(SIGABRT);
	install_shutdown_signal_handler(SIGFPE);		//?
//SIGKILL/SIGSTOP is not catchable anyway
	install_shutdown_signal_handler(SIGSEGV);		//?
	install_shutdown_signal_handler(SIGPIPE);		//?
	install_shutdown_signal_handler(SIGALRM);
	install_shutdown_signal_handler(SIGTERM);
//default action for SIGUSR1/SIGUSR2 is to terminate, so, until i have something better to do with them..
	install_shutdown_signal_handler(SIGUSR1);
	install_shutdown_signal_handler(SIGUSR2);
//ignored: SIGCHLD
#if _POSIX_VERSION >=200112L
	install_shutdown_signal_handler(SIGBUS);		//?
	install_shutdown_signal_handler(SIGPOLL);		//?
	install_shutdown_signal_handler(SIGSYS);		//?
	install_shutdown_signal_handler(SIGTRAP);		//?
//ignored: SIGURG
	install_shutdown_signal_handler(SIGVTALRM);
	install_shutdown_signal_handler(SIGXCPU);	//not sure this 1 is catchable..
	install_shutdown_signal_handler(SIGXFSZ);
#endif
#endif
#endif
//Now there are more non-standard signals who's default action is to terminate the process
// which we probably should look out for, but.... cba now. they shouldn't happen anyway (like some 99% of the list above)
}
struct mybuffer
{
	size_t buffer_size;
	char* buffer;
};
struct myrequest
{
	struct nbd_request nbdrequest;
	struct mybuffer mybuf;
};
struct myreply
{
	struct nbd_reply nbdreply __attribute((packed));
	struct mybuffer mybuf;
};
void print_request_data(const char *initial_message,
		const struct myrequest *request)
{
	return;
	printf("%s\n", initial_message);
	printf("request->magic: %i\n", NTOHL(request->nbdrequest.magic));
	printf("request->type: %i\n", NTOHL(request->nbdrequest.type));
	{
		uint64_t nhandle;
		static_assert(sizeof(nhandle) == sizeof(request->nbdrequest.handle),
				"if this fails, the the code around needs to get updated.");
		memcpy(&nhandle, request->nbdrequest.handle, sizeof(nhandle));
		printf("request->handle: %llu\n", NTOHLL(nhandle));
	}

	printf("request->from: %llu\n", NTOHLL(request->nbdrequest.from));
	printf("request->len: %u\n", NTOHL(request->nbdrequest.len));
}

