/*
 * ParseItAll.h
 *
 *  Created on: Feb 5, 2020
 *      Author: dad
 */

#ifndef PARSEITALL_H_
#define PARSEITALL_H_

#define FILECODE "data-filecode"
#define FILEID "id"
#define FILENAME "file name"
#define FILEIP "ip"
#define FILEDOWNLOADS "downloads"
#define FILEPWPROTECTED "password protected"
#define FILEDAYSEXP "days expiration"
#define FILEDOWNLOADSEXP "downloads expiration"
#define FILEACTIONS "actions"

typedef struct FileData
{
	char * FileName;
	char * FileCode;
} FileData;

typedef struct FileIndexStructure
{
	int           NumFIles;
	FileData **   FileIndex;
} FileIndexStructure;




#endif /* PARSEITALL_H_ */
