/*
	TECkit_Compile.c
	Copyright (c) 2002 SIL International.
	
	2004-03-12	updated to use v2.1 compiler
				added -u option to force UTF8 mode
*/

#include "TECkit_Compiler.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if __MWERKS__
#include "SIOUX.h"
#include "console.h"
#endif

#ifndef TARGET_RT_BIG_ENDIAN
#define TARGET_RT_BIG_ENDIAN	1
#endif

static CALLBACK
void
errFunc(void* usedData, char* msg, char* param, UInt32 line)
{
#pragma unused(usedData)

	fprintf(stderr, "%s", msg);
	if (param != 0)
		fprintf(stderr, ": \"%s\"", param);
	if (line != 0)
		fprintf(stderr, " at line %lu", line);
	fprintf(stderr, "\n");
}

int
main(int argc, char** argv)
{
	char*	progName = argv[0];
	char*	mapFileName = 0;
	char*	tecFileName = 0;
	int		errorCount = 0;
	char	compress = 1;
	char	form = kForm_Unspecified;
	
	if (*progName == 0)
		progName = "TECkit_Compile";
	
#if __MWERKS__ && __dest_os == __mac_os
	SIOUXSettings.asktosaveonclose = 0;
	argc = ccommand(&argv);
#endif

	while (--argc) {
		char*	arg = *++argv;
		if (arg[0] == '-' && strlen(arg + 1) == 1 && argc > 0) {
			switch (arg[1]) {
				case 'o':
					tecFileName = *++argv;
					--argc;
					break;
				case 'z':
					compress = 0;
					break;
				case 'u':
					form = kForm_UTF8;
					break;
				default:
					fprintf(stderr, "unknown option: -%c\n", arg[1]);
					++errorCount;
					break;
			}
		}
		else if (arg[0] != '-' && mapFileName == 0)
			mapFileName = arg;
		else {
			fprintf(stderr, "command line error at %s\n", arg);
			++errorCount;
		}
	}

	if (errorCount > 0 || mapFileName == 0) {
		fprintf(stderr, "\
Usage: %s mapping_description [-o compiled_table] [-z]\n\
\n\
    Required argument:\n\
        source mapping description (.map) file\n\
\n\
    Optional arguments:\n\
        -o file     output compiled table (.tec) file\n\
        -z          generate uncompressed table format\n\
        -u          read source text as UTF8 even if no BOM found\n\
", progName);
		return 1;
	}

	if (tecFileName == 0) {
		int	x = strlen(mapFileName);
		tecFileName = (char*)malloc(x + 5);
		if (tecFileName == 0)
			return 1;	// unlikely!
		strcpy(tecFileName, mapFileName);
		if (x > 4) {
			if (tecFileName[x - 4] == '.') {
				--x; tecFileName[x] = tolower(tecFileName[x]);
				--x; tecFileName[x] = tolower(tecFileName[x]);
				--x; tecFileName[x] = tolower(tecFileName[x]);
				if (strcmp(tecFileName + x, "map") == 0 || strcmp(tecFileName + x, "txt") == 0)
					tecFileName[x - 1] = 0;
				else
					strcpy(tecFileName, mapFileName);
			}
		}
		strcat(tecFileName, ".tec");
	}

	if (mapFileName != 0) {
		// compile the mapping
		FILE*	inFile = fopen(mapFileName, "rb");
		long	len;
		Byte*	compiledTable;
		UInt32	compiledSize;
		TECkit_Status	status;
		char*	txt;
		
		if (inFile == 0) {
			// try adding .map
			char*	mapFileName2 = (char*)malloc(strlen(mapFileName) + 5);
			if (mapFileName2 == 0)
				return 1;
			strcpy(mapFileName2, mapFileName);
			strcat(mapFileName2, ".map");
			inFile = fopen(mapFileName2, "rb");
			free(mapFileName2);
		}
		if (inFile == 0) {
			fprintf(stderr, "unable to open mapping file %s\n", mapFileName);
			return 1;
		}
		fseek(inFile, 0, SEEK_END);
		len = ftell(inFile);
		fseek(inFile, 0, SEEK_SET);
		
		txt = (char*)malloc(len);
		if (txt == 0) {
			fprintf(stderr, "not enough memory to read mapping file\n");
			return 1;	// not enough memory
		}

		fread(txt, len, 1, inFile);
		fclose(inFile);
		
		status = TECkit_CompileOpt(txt, len, &errFunc, 0, &compiledTable, &compiledSize,
									form | (compress ? kCompilerOpts_Compress : 0));

		free(txt);
		
		if (status == kStatus_NoError) {
			// save the compiled mapping
			FILE*	outFile;
			remove(tecFileName);
			outFile = fopen(tecFileName, "wb");
			if (outFile == 0) {
				fprintf(stderr, "unable to open output table file %s\n", tecFileName);
				return 1;
			}
			fwrite(compiledTable, compiledSize, 1, outFile);
			fclose(outFile);
			TECkit_DisposeCompiled(compiledTable);
		}
		else {
			fprintf(stderr, "compilation failed: status = %d\n", (int)status);
			return 1;
		}
		
		return 0;
	}

	return 1;
}
