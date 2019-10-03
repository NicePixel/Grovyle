#include <stdio.h>
#include <stdlib.h>
#include "file.h"

static const size_t READ_CHUNK = (1 << 21);

int
fileread(const char* path, char** buffer, size_t* buffersize)
{
	size_t size = 0;
	size_t used = 0;
	size_t n;
	char  *data = NULL, *temp;
	FILE* f;

	f = fopen(path, "rb");
	if (f == NULL || ferror(f))
	{
		fputs("Couldn't open file for reading.\n", stderr);
		return 1;
	}

	for (;;)
	{
		if (used + READ_CHUNK + 1 > size)
		{
			size = used + READ_CHUNK + 1;
			if (size <= used)
			{
				free(data);
				fclose(f);
				fputs("Overflow during file read operation.\n", stderr);
				return 1;
			}
			temp = realloc(data, size);
			if (temp == NULL)
			{
				free(data);
				fclose(f);
				fputs("Out of memory during file read operation.\n", stderr);
				return 1;
			}
			data = temp;
		}
		n = fread(data + used, 1, READ_CHUNK, f);
		if (n == 0)
		{
			break;
		}
		used += n;
	}
	if (ferror(f))
	{
		free(data);
		fclose(f);
		fputs("Failure during file read operation.\n", stderr);
		return 1;
	}
	temp = realloc(data, used + 1);
	if (temp == NULL)
	{
		free(data);
		fclose(f);
		fputs("Out of memory during file read operation.\n", stderr);
	}
	fclose(f);
	data = temp;
	data[used] = '\0';

	*buffer = data;
	*buffersize = used;
	return 0;
}
