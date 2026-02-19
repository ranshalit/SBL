/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2021, congatec GmbH. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the BSD 2-clause license which 
 * accompanies this distribution. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the BSD 2-clause license for more details.
 *
 * The full text of the license may be found at:        
 * http://opensource.org/licenses/BSD-2-Clause   
 *
 *---------------------------------------------------------------------------
 */

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "dmstobin.h"

#if !INI_USE_STACK
#include <stdlib.h>
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

#undef INI_ALLOW_MULTILINE
#undef INI_ALLOW_BOM
#undef INI_STOP_ON_FIRST_ERROR

#define INI_ALLOW_MULTILINE 0
#define INI_ALLOW_BOM 0
#define INI_STOP_ON_FIRST_ERROR 1 

// Maximum values set according to AMI token definitions
#define MAX_OEM_STRINGS 5
#define MAX_SYSCONFIG_STRINGS 5

#pragma pack(push, 1)
typedef struct {
    unsigned char Type;        // Type of SMBIOS table to be altered
    unsigned char Offset;      // Structure field offset, or string number for Type 11 and 12
    unsigned char Reserved;    // Reserved as 0x00
    unsigned char Flags;       // Reserved as 0x80
    unsigned char HdrLength;   // Reserved as 0x09 
    unsigned short Size;       // Data size without header, used only for UUID replacing, but should be correct for all entries
    unsigned short Handle;     // Reserved as 0xFFFF, which is "first suitable"
} TABLE_INFO;
#pragma pack(pop)

TABLE_INFO EntryHeader = {
    0x00,   // Will be replaced by correct table type
    0x00,   // Will be replaced by correct offset or string number
    0x00,
    0x80,
    0x09,
    0x0000, // Will be replaced by correct data size
    0xFFFF  // Special wildcard value
};

typedef struct {
    char* Name;
    unsigned char Num;
} NAME_TO_NUM_MAP_ENTRY;

NAME_TO_NUM_MAP_ENTRY TableNameToNumMap[] = {
    {"BIOS",             0},
    {"System",           1},
    {"BaseBoard",        2},
    {"Chassis",          3},
    {"OemString",        11},
    {"SysConfigOptions", 12},
};
const unsigned int TableNameToNumMapSize = sizeof(TableNameToNumMap)/sizeof(NAME_TO_NUM_MAP_ENTRY);

NAME_TO_NUM_MAP_ENTRY BiosNameToOffsetMap[] = {
    {"Vendor",       0x04},
    {"Version",      0x05},
    {"ReleaseDate",  0x08},
};
const unsigned int BiosNameToOffsetMapSize = sizeof(BiosNameToOffsetMap)/sizeof(NAME_TO_NUM_MAP_ENTRY);

NAME_TO_NUM_MAP_ENTRY SystemNameToOffsetMap[] = {
    {"Manufacturer", 0x04},
    {"Product",      0x05},
    {"Version",      0x06},
    {"SerialNum",    0x07},
    {"UUID",         0x08},
    {"SKU",          0x19},
    {"Family",       0x1A},
};
const unsigned int SystemNameToOffsetMapSize = sizeof(SystemNameToOffsetMap)/sizeof(NAME_TO_NUM_MAP_ENTRY);

NAME_TO_NUM_MAP_ENTRY BaseBoardNameToOffsetMap[] = {
    {"Manufacturer", 0x04},
    {"Product",      0x05},
    {"Version",      0x06},
    {"SerialNum",    0x07},
    {"TagNum",       0x08},
};
const unsigned int BaseBoardNameToOffsetMapSize = sizeof(BaseBoardNameToOffsetMap)/sizeof(NAME_TO_NUM_MAP_ENTRY);

NAME_TO_NUM_MAP_ENTRY ChassisNameToOffsetMap[] = {
    {"Manufacturer", 0x04},
    {"ChassisType",  0x05},
    {"Version",      0x06},
    {"SerialNum",    0x07},
    {"TagNum",       0x08},
    {"ChassisOEM",   0x0D},
};
const unsigned int ChassisNameToOffsetMapSize = sizeof(ChassisNameToOffsetMap)/sizeof(NAME_TO_NUM_MAP_ENTRY);

// Strings numbers are 1-based
unsigned char OemStringNum = 1;
unsigned char SysConfigOptionNum = 1;

// File pointers for input and output files
FILE* infile = NULL;
FILE* outfile = NULL;

/*---------------------------------------------------------------------------
 * Name: xdigitval
 * Desc:
 * Inp:  
 * Outp: 
 *---------------------------------------------------------------------------
 */
unsigned char xdigitval
(
	unsigned char digit
) 
{
    switch (digit) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a':
        case 'A': return 10;
        case 'b':
        case 'B': return 11;
        case 'c':
        case 'C': return 12;
        case 'd':
        case 'D': return 13;
        case 'e':
        case 'E': return 14;
        case 'f':
        case 'F': return 15;
    }
    return 0;
}									

/*---------------------------------------------------------------------------
 * Name: rstrip
 * Desc: Strip whitespace chars off end of given string, in place. Return s.
 * Inp:  s		- pointer to char
 * Outp: s		- last non-whitespace char
 *---------------------------------------------------------------------------
 */
static char* rstrip
(
	char* s
)
{
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

/*---------------------------------------------------------------------------
 * Name: lskip
 * Desc: Return pointer to first non-whitespace char in given string.
 * Inp:  s		- pointer to char
 * Outp: s		- first non-whitespace char
 *---------------------------------------------------------------------------
 */
static char* lskip
(
	const char* s
)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}

/*---------------------------------------------------------------------------
 * Name: find_char_or_comment
 * Desc: Return pointer to first char c or ';' comment in given string, or pointer to
 *       null at end of string if neither found. ';' must be prefixed by a whitespace
 *       character to register as a comment. 
 * Inp:  
 * Outp: 
 *---------------------------------------------------------------------------
 */
static char* find_char_or_comment
(
	const char* s, 
	char c
)
{
    int was_whitespace = 0;
    while (*s && *s != c && !(was_whitespace && *s == ';')) {
        was_whitespace = isspace((unsigned char)(*s));
        s++;
    }
    return (char*)s;
}

/*---------------------------------------------------------------------------
 * Name: strncpy0
 * Desc: Version of strncpy that ensures dest (size bytes) is null-terminated.
 * Inp:  
 * Outp: 
 *---------------------------------------------------------------------------
 */
static char* strncpy0
(
	char* dest,
	const char* src,
	size_t size
)
{
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

/*---------------------------------------------------------------------------
 * Name: ini_parse_stream
 * Desc: Same as ini_parse(), but takes an ini_reader function pointer instead of
 *		 filename. Used for implementing custom or string-based I/O.
 * Inp:  reader		- Typedef for prototype of fgets-style reader function
		 stream		- ini_reader function pointer
		 handler	- handler function
		 user		- user pointer
 * Outp: 0			- success
		 lineno		- line number in which an error occurred
		 -2			- heap memory could not be allocated
 *---------------------------------------------------------------------------
 */

int ini_parse_stream
(
	ini_reader reader,
	void* stream,
	ini_handler handler,
    void* user
)
{
    /* Uses a fair bit of stack (use heap instead if you need to) */
#if INI_USE_STACK
    char line[INI_MAX_LINE];
#else
    char* line;
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;

#if !INI_USE_STACK
    line = (char*)malloc(INI_MAX_LINE);
    if (!line) {
        return -2;
    }
#endif

    /* Scan through stream line by line */
    while (reader(line, INI_MAX_LINE, stream) != NULL) {
        lineno++;

        start = line;
#if INI_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
            /* Per Python ConfigParser, allow '#' comments at start of line */
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start && start > line) {
            /* Non-black line with leading whitespace, treat as continuation
               of previous name's value (as per Python ConfigParser). */
            if (!handler(user, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == '[') {
            /* A "[section]" line */
            end = find_char_or_comment(start + 1, ']');
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
            }
            else if (!error) {
                /* No ']' found on section line */
                error = lineno;
            }
        }
        else if (*start && *start != ';') {
            /* Not a comment, must be a name[=:]value pair */
            end = find_char_or_comment(start, '=');
            if (*end != '=') {
                end = find_char_or_comment(start, ':');
            }
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
                end = find_char_or_comment(value, '\0');
                if (*end == ';')
                    *end = '\0';
                rstrip(value);

                /* Valid name[=:]value pair found, call handler */
                strncpy0(prev_name, name, sizeof(prev_name));
                if (!handler(user, section, name, value) && !error)
                    error = lineno;
            }
            else if (!error) {
                /* No '=' or ':' found on name[=:]value line */
                error = lineno;
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }

#if !INI_USE_STACK
    free(line);
#endif

    return error;
}

/*---------------------------------------------------------------------------
 * Name: ini_parse_file
 * Desc: Same as ini_parse(), but takes a FILE* instead of filename. This doesn't
		 close the file when it's finished -- the caller must do that.
 * Inp:  file		- pointer to input file
		 handler	- handler function
		 user		- user pointer
 * Outp: 0			- success
		 lineno		- line number in which an error occurred
		 -1			- output file could not be opened
		 -2			- heap memory could not be allocated
 *---------------------------------------------------------------------------
 */

int ini_parse_file
(
	FILE* file,
	ini_handler handler,
	void* user
)
{
	int success = 0;
	
	// Ensure default initialization for EntryHeader fields on each run.
	EntryHeader.Type = 0x00;
	EntryHeader.Offset = 0x00;
	EntryHeader.Reserved = 0x00;
	EntryHeader.Flags = 0x80;
	EntryHeader.HdrLength = 0x09;
	EntryHeader.Size = 0x0000;
	EntryHeader.Handle = 0xFFFF;

	OemStringNum = 1;
	SysConfigOptionNum = 1;

	// File pointers for input and output files	
	infile = NULL;
	outfile = NULL;

	outfile = fopen("tmp.bin", "wb");
	if (outfile == NULL) {
        //perror("Can't create or open output file");
        return -1;
    } 
    success = ini_parse_stream((ini_reader)fgets, file, handler, user);
	fclose(outfile);
	return success;
}

/*---------------------------------------------------------------------------
 * Name: ini_parse
 * Desc: Parse given INI-style file. May have [section]s, name=value pairs
		 (whitespace stripped), and comments starting with ';' (semicolon). Section
		 is "" if name=value pair parsed before any section heading. name:value
		 pairs are also supported as a concession to Python's ConfigParser.
		 For each name=value pair parsed, call handler function with given user
		 pointer as well as section, name, and value (data only valid for duration
	     of handler call). Handler should return nonzero on success, zero on error.
		 Returns 0 on success, line number of first error on parse error (doesn't
		 stop on first error), -1 on file open error, or -2 on memory allocation
		 error (only when INI_USE_STACK is zero).
 * Inp:  filename		- filename pointer
		 handler		- handler function
		 user			- user pointer
 * Outp: 0				- success
		 -1				- file could not be opened
 *---------------------------------------------------------------------------
 */
/*
int ini_parse
(
	const char* filename,
	ini_handler handler,
	void* user
)
{
    FILE* file;
    int error;

    file = fopen(filename, "r");
    if (!file)
        return -1;
    error = ini_parse_file(filename, handler, user);
    fclose(file);
    return error;
}
*/
/*---------------------------------------------------------------------------
 * Name: convert
 * Desc: handler function for converting dms to bin files
 * Inp:  user			- user pointer
		 section		- section pointer
		 name			- name pointer
		 value			- value pointer
 * Outp: 1				- success
		 0				- failure
 *---------------------------------------------------------------------------
 */

int convert
(
	void* user,
	const char* section,
	const char* name,
    const char* value
)
{
    unsigned int i, j;
    unsigned char found = 0;

    
    // Check section
    for (i = 0; i < TableNameToNumMapSize; ++i) {
        if (strcmp(section, TableNameToNumMap[i].Name) == 0) {
            // Correct section found 
            found = 1;
            EntryHeader.Type = TableNameToNumMap[i].Num;
        }
    }
    if (!found) {
        printf("convert: unknown section name '%s'\n", section);
        return 0;
    }
    
    found = 0;
    switch (EntryHeader.Type) {
        case 0:   // BIOS
            // Check name
            for (i = 0; i < BiosNameToOffsetMapSize; ++i) {
                if (strcmp(name, BiosNameToOffsetMap[i].Name) == 0) {
                    // Correct name found 
                    found = 1;
                    // Fill entry header
                    EntryHeader.Offset = BiosNameToOffsetMap[i].Num;
                    EntryHeader.Size = (unsigned short)strlen(value) + 1;
                    
                    // Special case of release date
                    if (EntryHeader.Offset== 0x08) {
                        // Date format must be MM/DD/YYYY according to SMBIOS 2.3+ spec
                        unsigned int month = 10*xdigitval(value[0]) + xdigitval(value[1]);
                        unsigned int day   = 10*xdigitval(value[3]) + xdigitval(value[4]);
                        unsigned int year =  1000*xdigitval(value[6]) + 100*xdigitval(value[7]) + 10*xdigitval(value[8]) + xdigitval(value[9]);
                        if (EntryHeader.Size != 11 ||
                            !isdigit(value[0]) ||   
                            !isdigit(value[1]) ||
                            value[2] != '/' ||
                            !isdigit(value[3]) ||
                            !isdigit(value[4]) ||
                            value[5] != '/' ||
                            !isdigit(value[6]) ||
                            !isdigit(value[7]) ||
                            !isdigit(value[8]) ||
                            !isdigit(value[9]) ||
                            month < 1 ||
                            month > 12 ||
                            day < 1 ||
                            day > 31 ||
                            year < 1900) {
                                printf("convert: BIOS release date format must be MM/DD/YYYY\n");
                                return 0;
                            }
                    }
                    
                    // Write header and data to output file
                    if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                        fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                        printf("convert: can't write to output file\n");
                        return 0;
                    }
                    
                    // No need to search further
                    break;
                }
            }
        break;
        case 1:   // System
            // Check name
            for (i = 0; i < SystemNameToOffsetMapSize; ++i) {
                if (strcmp(name, SystemNameToOffsetMap[i].Name) == 0) {
                    // Correct name found 
                    found = 1;
                    // Fill entry header
                    EntryHeader.Offset = SystemNameToOffsetMap[i].Num;
					EntryHeader.Size = (unsigned short)strlen(value) + 1;
                    
                    // Special case of UUID
                    if (EntryHeader.Offset == 0x08) {
                        unsigned char UUID[16] = {0x00};
                        
						// Remove trailing zero
						EntryHeader.Size--;

                        // UUID must consist of positive even number of hex digits up to 32
                        if (EntryHeader.Size > 32) {
                            printf("convert: UUID can't have more than 32 digits\n");
                            return 0;
                        }
                        if (EntryHeader.Size % 2) {
                            printf("convert: UUID can't have odd number of digits\n");
                            return 0;
                        }
                        for (j = 0; j < EntryHeader.Size; ++j) {
                            if(!isxdigit(value[j])) {
                                printf("convert: UUID should only consist of hex digits\n");
                                return 0;
                            }
                        }
                        
                        // Convert UUID to binary
                        for (j = 0; j < (unsigned int)(EntryHeader.Size/2); ++j) {
                            UUID[j] = 0x10*xdigitval(value[2*j]) + xdigitval(value[2*j+1]);
                        }
                        
                        EntryHeader.Size = sizeof(UUID);
                        // Write header and data to output file
                        if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                            fwrite(&UUID, EntryHeader.Size, 1, outfile) != 1) {
                            printf("convert: can't write to output file\n");
                            return 0;
                        }
                    }
                    else {
                        // Write header and data to output file
                        if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                            fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                            printf("convert: can't write to output file\n");
                            return 0;
                        }
                    }
                    
                    // No need to search further
                    break;
                }
            }
        break;
        case 2:   // BaseBoard
            // Check name
            for (i = 0; i < BaseBoardNameToOffsetMapSize; ++i) {
                if (strcmp(name, BaseBoardNameToOffsetMap[i].Name) == 0) {
                    // Correct name found 
                    found = 1;
                    // Fill entry header
                    EntryHeader.Offset = BaseBoardNameToOffsetMap[i].Num;
					EntryHeader.Size = (unsigned short)strlen(value) + 1;
                    
                    // Write header and data to output file
                    if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                        fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                        printf("convert: can't write to output file\n");
                        return 0;
                    }
                    
                    // No need to search further
                    break;
                }
            }
        break;
        case 3:   // Chassis
            // Check name
            for (i = 0; i < ChassisNameToOffsetMapSize; ++i) {
                if (strcmp(name, ChassisNameToOffsetMap[i].Name) == 0) {
                    // Correct name found 
                    found = 1;
                    // Fill entry header
                    EntryHeader.Offset = ChassisNameToOffsetMap[i].Num;
					EntryHeader.Size = (unsigned short)strlen(value) + 1;
                    
                    // Special handling for type
                    if (EntryHeader.Offset == 0x05) {
                        unsigned char type;

						// Remove trailing zero
						EntryHeader.Size--;

                        //Type must be exactly 2 hex digits
                        if (EntryHeader.Size != 2) {
                            printf("convert: chassis type must be 2 hex digits long\n");
                            return 0;
                        }
                        if (!isxdigit(value[0]) || !isxdigit(value[1])) {
                            printf("convert: chassis type must consist of hex digits\n");
                            return 0;
                        }
                        type = 0x10*xdigitval(value[0]) + xdigitval(value[1]);
                        EntryHeader.Size = 1;
                        // Write header and data to output file
                        if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                            fwrite(&type, EntryHeader.Size, 1, outfile) != 1) {
                            printf("convert: can't write to output file\n");
                            return 0;
                        }
                    }
                    // Special handling for OEM value
                    else if (EntryHeader.Offset == 0x0D) {
                        unsigned int oem;

						// Remove trailing zero
						EntryHeader.Size--;

                        //OEM value must be exactly 8 hex digits
                        if (EntryHeader.Size != 8) {
                            printf("convert: chassis OEM value must be 8 hex digits long\n");
                            return 0;
                        }
                        if (!isxdigit(value[0]) || !isxdigit(value[1]) || !isxdigit(value[2]) || !isxdigit(value[3]) ||
                            !isxdigit(value[4]) || !isxdigit(value[5]) || !isxdigit(value[6]) || !isxdigit(value[7])) {
                            printf("convert: chassis OEM value must consist of hex digits\n");
                            return 0;
                        }
						if (sscanf(value, "%x", &oem) != 1) {
                            printf("convert: error getting chassis OEM value\n");
                            return 0;
                        }
                        EntryHeader.Size = sizeof(oem);
                        // Write header and data to output file
                        if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                            fwrite(&oem, EntryHeader.Size, 1, outfile) != 1) {
                            printf("convert: can't write to output file\n");
                            return 0;
                        }
                    }
                    else {
                        // Write header and data to output file
                        if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                            fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                            printf("convert: can't write to output file\n");
                            return 0;
                        }
                    }
                    // No need to search further
                    break;
                }
            }
        break;
        case 11:  // OemString
            if (OemStringNum > MAX_OEM_STRINGS) {
                printf("convert: too many OEM strings defined\n");
                return 0;
            }
            
            if (strcmp(name, "String") == 0) {
                // Correct name found 
                found = 1;
                // Fill entry header
                EntryHeader.Offset = OemStringNum++;
				EntryHeader.Size = (unsigned short)strlen(value) + 1;
                // Write header and data to output file
                if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                    fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                    printf("convert: can't write to output file\n");
                    return 0;
                }
            }
        break;
        case 12:  // SysConfigOptions
			if (SysConfigOptionNum > MAX_SYSCONFIG_STRINGS) {
                printf("convert: too many system configuration option strings defined\n");
                return 0;
            }
            
            if (strcmp(name, "String") == 0) {
                // Correct name found 
                found = 1;
                // Fill entry header
                EntryHeader.Offset = SysConfigOptionNum++;
				EntryHeader.Size = (unsigned short)strlen(value) + 1;
                // Write header and data to output file
                if (fwrite(&EntryHeader, sizeof(TABLE_INFO), 1, outfile) != 1 ||
                    fwrite(value, EntryHeader.Size, 1, outfile) != 1) {
                    printf("convert: can't write to output file\n");
                    return 0;
                }
            }
        break;
        default: // Some unknown stuff, just in case
            printf("convert: unknown table number %d\n", EntryHeader.Type);
            return 0;
    }
    
    if (!found) {
        printf("convert: unknown setting name '%s' in section '%s'\n", name, section);
        return 0;
    }
    
    return 1;
}											