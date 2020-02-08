/*
 * ParseItAll.cpp
 *
 *  Created on: Feb 5, 2020
 *      Author: dad
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ParseItAll.h"
#include "parselib.h"

//FileIndexStructure FileList;
FileData ** FileIndexpp;
FileData * FileIndexp;

int ParseItAll(char * src, FileIndexStructure * FileList, int * isettings)
{
	char * allsmall;
	char * eosSmall;
	char * eosTall;
	int indexTable;
	int lenTable;
	int lenTot;
	int indexThead;
	int lenThead;
	allsmall = ToAllLower ( src );
	lenTot = strlen(src);
	eosSmall = allsmall + lenTot;
	eosTall = src + lenTot;
	FileList->NumFIles = 0;
	FileIndexpp = (FileData**)malloc(sizeof(FileData*));
	FileList->FileIndex = FileIndexpp;


	char * StartTable =  ParseSubString ( allsmall , TABLE_START, TABLE_END, eosSmall, &lenTable, &indexTable );
	char * StartThead =  ParseSubString ( StartTable , TABLE_HEADER_START, TABLE_HEADER_END, StartTable+lenTable, &lenThead, &indexThead );
	int IndexToFileName = -1;
	char * StartTh;
	StartTh = StartThead;
	char * NextTh;
	int indexTh;
	int lenTh;
	int icount = 0;
	while (IndexToFileName == -1) {
		NextTh =  ParseSubString ( StartTh , TABLE_HEADER_ITEM_START, TABLE_HEADER_ITEM_END, StartThead+lenThead, &lenTh, &indexTh );
		if ( strncmp ( FILENAME, (const char *) NextTh, strlen(FILENAME) ) == 0 ) IndexToFileName = icount;
		icount++;
		StartTh = NextTh+lenTh;
	}
	fprintf(stderr,"File Name is index %i\n",IndexToFileName);
	int lenBody;
	int indexBody;
	char * StartBody =  ParseSubString ( StartTh , TABLE_BODY_START, TABLE_BODY_END, StartTable+lenTable, &lenBody, &indexBody );
	char * StartRow;
	StartRow = StartBody;
	int lenRow;
	int indexRow;
	int i;
	char * NextRow = StartRow;
	char * NextTd;
	int lenTd;
	int indexTd;
	char * FileName;
	char * FileCode;
	int lenDataFileCode;
	int indexDataFileCode;
	char * DataFileCode;
	int AbsIndex;
	while ( NextRow != NULL ){
		NextTd =  ParseSubString ( NextRow , TABLE_ROW_START, TABLE_ROW_END, StartBody+lenBody, &lenRow, &indexRow );
		if ( NextTd == NULL ) break;
		DataFileCode =  ParseAttibute ( NextRow , DATA_FILE_CODE, "\"", StartBody+lenBody, &lenDataFileCode, &indexDataFileCode );
		NextRow = NextTd;
		for (i=0;i<IndexToFileName+1;i++){
			NextTd =  ParseSubString ( NextTd , TABLE_DATA_ITEM_START, TABLE_DATA_ITEM_END, StartBody+lenBody, &lenTd, &indexTd );
		}
		FileIndexp = (FileData*)malloc(sizeof(FileData));
		FileList->NumFIles++;
		FileIndexpp = (FileData**)realloc(FileIndexpp,sizeof(FileData*)*(FileList->NumFIles+2));
		FileList->FileIndex=FileIndexpp;
		FileName = (char *)malloc(lenTd);
		AbsIndex = (int)(NextTd-allsmall);
		strncpy(FileName,src+AbsIndex,lenTd-1);
		*(FileName+lenTd-1)='\000';
		fprintf(stderr,"Found file %s : ",FileName);
		FileCode = (char *)malloc(lenDataFileCode);
		strncpy(FileCode,DataFileCode,lenDataFileCode-1);
		*(FileCode+lenDataFileCode-1)='\000';
		fprintf(stderr,"Data File Code =  %s\n",FileCode);
		FileIndexp->FileName=FileName;
		FileIndexp->FileCode=FileCode;
		FileIndexpp[FileList->NumFIles]=FileIndexp;
	}
// find PasswdSettings.txt
	for (i=0;i<FileList->NumFIles;i++) {
		FileIndexp=FileIndexpp[i+1];
		if ( strcmp(FileIndexp->FileName,"PasswdSettings.txt") == 0 ) {
			fprintf(stderr,"found PasswdSettings.txt...\n");
			*isettings = i+1;
		}
	}
	free(allsmall);
	return (0);
}

void FreeUpHeap(FileIndexStructure* FileList)
{
	int i;
	///   free all allocated structs
	for (i=0;i<FileList->NumFIles;i++) {
		FileIndexp=FileIndexpp[i+1];
		free(FileIndexp->FileName);
		free(FileIndexp->FileCode);
	}
	free(FileIndexpp);
}
