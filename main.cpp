/*
 * main.cpp
 *
 *  Created on: Jan 24, 2020
 *      Author: dad
 */


#include <string.h>
#ifdef WINDOZE
#else
#include <execinfo.h>
#include <err.h>
#include <unistd.h>
#include <error.h>
#endif
#include <errno.h>
#include <stdio.h>

#include "classes.h"
#include "ParseItAll.h"

using std::string;

void init(const int argc, char **argv, const bool onlyapitests);
void Downloadcacapitests();
extern int ParseItAll(char * src, FileIndexStructure * FileIndexData, int * isettings);

extern std::string curlprettyerror(const string name, const CURLcode errnum);

string argv_username;
string argv_password;
FileIndexStructure FileIndexData;

extern FileData ** FileIndexpp;
extern FileData * FileIndexp;

int main(int argc, char **argv)
{
	init(argc, argv, 1);

	Downloadcacapitests();

	return 0;
}

void init(const int argc, char **argv, const bool onlyapitests)
{
	{
		curl_version_info_data* curlv = curl_version_info( CURLVERSION_NOW);
		if (!(curlv->features & CURL_VERSION_SSL))
		{
			fprintf(stderr, "error: libcurl was not compiled with SSL/TLS support, which cacdrive requires (TLS is required to login @cac cloud storage system)\n");
			exit(1);
		}

	}
	//read config file
	{
		if (argc != 3)
		{
			fprintf(stderr, "usage: %s username password\n",argv[0]);
			exit(EXIT_FAILURE);
		}
		argv_username = argv[1];
		argv_password = argv[2];
		{
			CURLcode c = curl_global_init(CURL_GLOBAL_ALL);
			if (c != CURLE_OK)
			{
				throw std::runtime_error(
						curlprettyerror("curl_global_init()", c));
			}
		}
	}
}



void Downloadcacapitests()
{
	using namespace std::chrono;
	milliseconds ms = duration_cast<milliseconds>(
			system_clock::now().time_since_epoch());
#define ugly(){milliseconds noww=duration_cast<milliseconds >(system_clock::now().time_since_epoch());fprintf(stdout,"%fs ",double((noww-ms).count())/1000);ms=duration_cast<milliseconds >(system_clock::now().time_since_epoch());}
	string tmp;
	string tmp2;
	string uploadcontent = "Hello world!";
	fprintf(stdout, "downloadcacapi tests..\n logging in: ");
	fflush(stdout);
	Downloadcacapi* cacks = new Downloadcacapi(argv_username, argv_password);
	ugly()
	fprintf(stdout, "done! cookies: ");
	for (auto& cookie : cacks->get_cookies())
	{
//		fprintf(stdout, " %s - ",cookie);
		fprintf(stdout, " %s - ",cookie.c_str());
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "listing files test: ");
	fflush(stdout);
	tmp = cacks->list_files();
	ugly()
	FILE *tf;
	tf = fopen("list.txt","w");
	fprintf(tf,"%s",tmp.c_str());
	fclose(tf);
	fprintf(stdout, "done! file list outut written to list.txt\n");
/////////////////   parse the list file here   /////////////////////
	char * src = (char *)tmp.c_str();
	int isettings;
	ParseItAll( src, &FileIndexData, &isettings );
	FileIndexpp = FileIndexData.FileIndex;
	FileIndexp=FileIndexpp[isettings];
	tmp = FileIndexp->FileCode;
	tmp2 = cacks->download(tmp);
	tf = fopen("PasswdSettings.txt","w");
	fprintf(tf,"%s",tmp2.c_str());
	fclose(tf);
	fprintf(stdout, "done! PasswdSettings.txt downloaded and saved\n");

/*	fprintf(stdout, "uploading test.txt: ");
	fflush(stdout);
	tmp = cacks->upload(uploadcontent, "test.txt");
	ugly()
	fprintf(stdout, "done! upload id: %s\n",tmp.c_str());
	for (int i = 0; i < 2; ++i)
	{
		fprintf(stdout, "downloading what we just uploaded, using the single download api.. ");
		fflush(stdout);
		tmp2 = cacks->download(tmp);
		ugly()
		fprintf(stdout, "done! contents: %s %s\n",tmp2.c_str(),(tmp2 == uploadcontent ?
				(" (content is intact.)") :
				(" (WARNING: content is corrupt!!!)"))
				);
		tf = fopen("test.txt","w");
		fprintf(tf,"%s",tmp2.c_str());
		fclose(tf);
	}
	fprintf(stdout, "deleting upload using synchronous api: ");
	fflush(stdout);
	cacks->delete_upload(tmp);
	ugly()
	fprintf(stdout, "done!\n");
*/

///
	fprintf(stdout, "logging out: ");
	fflush(stdout);
	delete cacks;
	ugly()
	fprintf(stdout, "done!\n");
#undef ugly
}



