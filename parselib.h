/*
 * parselib.h
 *
 *  Created on: Feb 5, 2020
 *      Author: dad
 */

#ifndef PARSELIB_H_
#define PARSELIB_H_

#define TABLE_START "<table"
#define TABLE_END   "</table"
#define TABLE_ROW_START "<tr"
#define TABLE_ROW_END   "</tr"
#define TABLE_HEADER_START "<thead"
#define TABLE_HEADER_END   "</thead"
#define TABLE_BODY_START   "<tbody"
#define TABLE_BODY_END     "</tbody"
#define TABLE_HEADER_ITEM_START   "<th"
#define TABLE_HEADER_ITEM_END     "</th"
#define TABLE_DATA_ITEM_START   "<td"
#define TABLE_DATA_ITEM_END     "</td"
#define DATA_FILE_CODE		"data-filecode=\""

extern char * cStartOfElement (char * source , const char * element, const char * eos);
extern char * cEndOfElement (char * source , const char * element, const char * eos);
extern char * ParseSubString ( char * source , const char * startelement, const char * endelement, const char * eos, int * SubStringLength, int * SubStringIndex );
extern char * ToAllLower ( char * source );
extern char * cStartOfAttribute (char * source , const char * element, const char * eos);
extern char * ParseAttibute ( char * source , const char * startelement, const char * endelement, const char * eos, int * SubStringLength, int * SubStringIndex );


#endif /* PARSELIB_H_ */
