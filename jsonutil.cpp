#include "jsonutil.h"

void json_seekbuffer(JsonContext* context, int posmod) {

	if (posmod == 0) {
		return;
	}
	else if (context->bufferFile) {

		fflush(context->bufferFile);
		fseek(context->bufferFile, posmod, SEEK_CUR);
	}
	else {
		if (context->bufferLength + posmod <= 0) {
			context->bufferLength = 0;
		}
		else if (context->bufferLength + posmod > context->bufferSize) {
			context->bufferLength = context->bufferSize;
		}
		else {
			context->bufferLength += posmod;
		}
	}
}

void json_bail(lua_State *L, JsonContext* context, const char * err) {

	if (context->bufferFile) {
		fclose(context->bufferFile);

		// Delete the file on error
		if (err && context->fileName) {
			remove(context->fileName);
		}
	}

	if (context->readFile) {
		fclose(context->readFile);
	}

	if (context->buffer) {
		free(context->buffer);
	}

	if (context->fileName) {
		free(context->fileName);
	}

	if (context->antiRecursion) {
		free(context->antiRecursion);
	}

	memset(context, 0, sizeof(JsonContext));

	if (err) {
		luaL_error(L, err);
	}
}

void json_append(const char * data, size_t len, lua_State *L, JsonContext* context) {

	if (context->bufferFile) {

		if (fwrite(data, sizeof(char), len, context->bufferFile) != len) {
			json_bail(L, context, "Failed to write to file");
		}
	}
	else {

		if (context->bufferLength + (len + 1) > context->bufferSize) {

			size_t newSize = (size_t)pow(2.0, (++context->resultReallocStep) > 20 ? 20 : context->resultReallocStep);

			if (newSize < JSONINITBUFFERSIZE) {
				newSize = JSONINITBUFFERSIZE;
			}

			if (newSize < len + 1) {
				newSize = len + 1;
			}

			newSize = context->bufferSize + (newSize * sizeof(char));

			void * temp = realloc(context->buffer, newSize);
			if (!temp) {

				json_bail(L, context, "Out of memory");
			}
			else {
				context->buffer = (char*)temp;
				context->bufferSize = newSize;
			}
		}

		memcpy(&context->buffer[context->bufferLength], data, (len * sizeof(char)));
		context->bufferLength += (len * sizeof(char));
		context->buffer[context->bufferLength] = '\0';
	}
}

unsigned int table_crc32(const unsigned char* data, int size)
{
	unsigned int r = 0xFFFFFFFF;
	const unsigned char* end = data + size;
	unsigned int t;

	while (data < end)
	{
		r ^= *data++;

		for (int i = 0; i < 8; i++)
		{
			t = ~((r & 1) - 1);
			r = (r >> 1) ^ (0xEDB88320 & t);
		}
	}

	return ~r;
}

bool json_addtoantirecursion(unsigned int id, JsonContext* context) {

	int idx = -1;
	if (context->antiRecursion) {
		for (size_t i = 0; i < context->antiRecursionSize; i++)
		{
			if (context->antiRecursion[i] == 0) {
				idx = i;
				break;
			}
		}
	}

	if (idx == -1) {
		void*temp = realloc(context->antiRecursion, (context->antiRecursionSize * sizeof(unsigned int)) + (10 * sizeof(unsigned int)));
		if (!temp) {
			return false;
		}

		context->antiRecursion = (unsigned int*)temp;
		idx = context->antiRecursionSize;
		memset(&context->antiRecursion[context->antiRecursionSize], 0, 10 * sizeof(unsigned int));
		context->antiRecursionSize += 10;
	}

	context->antiRecursion[idx] = id;

	return true;
}

bool json_existsinantirecursion(unsigned int id, JsonContext* context) {

	if (context->antiRecursion) {
		for (size_t i = 0; i < context->antiRecursionSize; i++)
		{
			if (context->antiRecursion[i] == id) {
				return true;
			}
		}
	}

	return false;
}

void json_removefromantirecursion(unsigned int id, JsonContext* context) {

	if (context->antiRecursion) {
		for (size_t i = 0; i < context->antiRecursionSize; i++)
		{
			if (context->antiRecursion[i] == id) {
				context->antiRecursion[i] = 0;
				return;
			}
		}
	}
}