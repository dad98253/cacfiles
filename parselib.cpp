/*
 * parselib.cpp
 *
 *  Created on: Feb 5, 2020
 *      Author: dad
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char * ToAllLower ( char * source )
{
	char * temp;
	char c;
	int SourceLen;
	int i = 0;
	if ( source == NULL ) return (NULL);
	SourceLen = strlen( source );
	if ( SourceLen == 0 ) return (NULL);
	temp = (char*) malloc(SourceLen+1);
	if ( temp == NULL ) return (NULL);
	strcpy(temp,source);
	while (temp[i])
	{
		c = temp[i];
		temp[i] = tolower(c);
		i++;
	}
	return (temp);
}

char * cStartOfElement (char * source , const char * element, const char * eos)
{
	if ( source == NULL || element == NULL || eos == NULL ) return(NULL);
	if ( source > eos ) return (NULL);
	int sizeofelement;
	sizeofelement = strlen(element);
	if ( (int)(eos - source) < sizeofelement ) return (NULL);
	char * temp;
	temp = strstr(source,element);
	if (temp == NULL) return (temp);
// if what we found is beyond our search range, return NULL
	if ( temp > eos ) {
		temp = NULL;
		return (temp);
	}
	if ( isspace(*(temp+sizeofelement)) ) return (temp);
	if ( *(temp+sizeofelement) == '>' ) return (temp);
	temp = NULL;
	return (temp);
}

char * cEndOfElement (char * source , const char * element, const char * eos)
{
	if ( source == NULL || element == NULL || eos == NULL ) return(NULL);
	if ( source > eos ) return (NULL);
	int sizeofelement;
	sizeofelement = strlen(element);
	if ( (int)(eos - source) < sizeofelement ) return (NULL);
	char * temp;
	char RightAngle[] = ">";
	temp = strstr(source,element);
	if (temp == NULL) return (temp);
// if what we found is beyond our search range, return NULL
	if ( temp > eos ) {
		temp = NULL;
		return (temp);
	}
// check for valid element - could be some other elemet that starts with our element string
	if ( !(isspace(*(temp+sizeofelement))) && ( *(temp+sizeofelement) != '>' ) ) {
		temp = NULL;
		return (temp);
	}
// find the terminating '>'
	char * temp2;
	temp2 = strstr(temp,RightAngle);
	return (temp2);
}

char * cStartOfAttribute (char * source , const char * element, const char * eos)
{
	if ( source == NULL || element == NULL || eos == NULL ) return(NULL);
	if ( source > eos ) return (NULL);
	int sizeofelement;
	sizeofelement = strlen(element);
	if ( (int)(eos - source) < sizeofelement ) return (NULL);
	char * temp;
	temp = strstr(source,element);
	if (temp == NULL) return (temp);
// if what we found is beyond our search range, return NULL
	if ( temp > eos ) {
		temp = NULL;
		return (temp);
	}
	return (temp);
}

char * ParseSubString ( char * source , const char * startelement, const char * endelement, const char * eos, int * SubStringLength, int * SubStringIndex )
{
	char * temp;
	char * temp2;
	temp = cEndOfElement (source , startelement, eos);
	if (temp == NULL) return (temp);
	temp2 = cStartOfElement (temp+1 , endelement, eos);
	if (temp2 == NULL) return (temp);
	*SubStringLength = (int)(temp2 - temp);
	*SubStringIndex  = (int) ( temp+1-source );
	return (temp+1);
}

char * ParseAttibute ( char * source , const char * startelement, const char * endelement, const char * eos, int * SubStringLength, int * SubStringIndex )
{
	char * temp;
	char * temp2;
	temp = cStartOfAttribute (source , startelement, eos);
	if (temp == NULL) return (temp);
	temp+=(strlen(startelement)-1);
	temp2 = cStartOfAttribute (temp+1 , endelement, eos);
	if (temp2 == NULL) return (temp);
	*SubStringLength = (int)(temp2 - temp);
	*SubStringIndex  = (int) ( temp+1-source );
	return (temp+1);
}


