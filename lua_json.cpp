#include "lua_json.h"
#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "math.h"

#define JSONINITBUFFERSIZE 1024
#define JSONANTIRECURSIONINITSIZE 10; 

static int jsonNull = -1;
static unsigned int jsonNullValue = 0;
static int jsonEmpty = -1;
static unsigned int jsonEmptyValue = 0;

void lua_jsonencodetable(lua_State *L, JsonContext* context);
void lua_readtable(lua_State *L, JsonContext* context);

void json_end(lua_State *L, JsonContext* context, const char * err) {

	if (context->result) {
		free(context->result);
	}

	if (context->antiRecursion) {
		free(context->antiRecursion);
	}

	if (err) {
		luaL_error(L, err);
	}
}

void json_append(const char * data, size_t len, lua_State *L, JsonContext* context) {

	if (context->resultLength + (len + 1) > context->resultSize) {

		size_t newSize = context->resultSize + ((len + JSONINITBUFFERSIZE) * sizeof(char));
		void * temp = realloc(context->result, newSize);
		if (!temp) {
			json_end(L, context, "Out of memory");
		}
		else {
			context->result = (char*)temp;
			context->resultSize = newSize;
		}
	}

	memcpy(&context->result[context->resultLength], data, (len * sizeof(char)));
	context->resultLength += (len * sizeof(char));
	context->result[context->resultLength] = '\0';
}

void json_appendstring(lua_State *L, JsonContext* context) {

	size_t stackSize = lua_gettop(L);
	size_t len;
	const char * str = luaL_tolstring(L, -1, &len);
	char hex[7];

	json_append("\"", 1, L, context);
	for (size_t i = 0; i < len; i++)
	{
		switch (str[i]) {
		case '\"':
			json_append("\\\"", 2, L, context);
			break;
		case '\\':
			json_append("\\\\", 2, L, context);
			break;
		case '/':
			json_append("\\/", 2, L, context);
			break;
		case '\n':
			json_append("\\n", 2, L, context);
			break;
		case '\r':
			json_append("\\r", 2, L, context);
			break;
		case '\0':
			json_append("\\0", 2, L, context);
			break;
		default:

			if ((str[i]) < ' ') {
				sprintf(hex, "\\u%04x", str[i]);
				json_append(hex, 6, L, context);
			}
			else {
				json_append(&str[i], 1, L, context);
			}
			break;
		}
	}
	json_append("\"", 1, L, context);

	if (lua_gettop(L) != stackSize) {
		lua_settop(L, stackSize);
	}
}

void json_appendnumber(lua_State *L, JsonContext* context) {

	double numb = lua_tonumber(L, -1);
	char number[50];

	if (isfinite(numb)) {
		
		long long rounded = llround(numb);

		if (rounded == numb) {
			sprintf(number, "%lld", rounded);
		}
		else {
			sprintf(number, "%f", numb);
		}
	}
	else if (isnan(numb)) {
		strcpy(number, "null");
	}
	else {

		switch (isinf(numb)) {
		case -1:
			strcpy(number, "-1e+9999");
			break;
		case 1:
			strcpy(number, "1e+9999");
			break;
		default:
			strcpy(number, "null");
			break;
		}
	}

	json_append(number, strlen(number), L, context);
}

void json_appendvalue(lua_State *L, JsonContext* context) {

	switch (lua_type(L, -1)) {
	case LUA_TTABLE:
		lua_jsonencodetable(L, context);
		break;
	case LUA_TNUMBER:
		json_appendnumber(L, context);
		break;
	case LUA_TBOOLEAN:
		json_append(lua_toboolean(L, -1) > 0 ? "true" : "false", lua_toboolean(L, -1) > 0 ? 4 : 5, L, context);
		break;
	default:
		if (lua_isnoneornil(L, -1)) {
			json_append("null", 4, L, context);
		}
		else {
			json_appendstring(L, context);
		}
		break;
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

void lua_jsonencodetable(lua_State *L, JsonContext* context) {

	size_t len;
	const char * rawid = luaL_tolstring(L, -1, &len);
	lua_pop(L, 1);
	lua_len(L, -1);
	int size = lua_tointeger(L, -1);
	lua_pop(L, 1);

	unsigned int id = table_crc32((const unsigned char*)rawid, len);

	if (id == jsonNullValue && context->resultLength > 0) {
		json_append("null", 4, L, context);
		return;
	}
	else if (id == jsonEmptyValue) {
		json_append("{}", 2, L, context);
		return;
	}

	if (!context->antiRecursion || context->antiRecursionSize <= context->antiRecursionLength) {

		size_t newsize = context->antiRecursionSize + JSONANTIRECURSIONINITSIZE
			void * temp = realloc(context->antiRecursion, newsize * sizeof(unsigned int));

		if (!temp) {
			json_end(L, context, "Out of memory");
			return;
		}
		else {
			context->antiRecursion = (unsigned int*)temp;
			context->antiRecursionSize = newsize;
		}
	}

	size_t count = 0;

	for (size_t i = 0; i < context->antiRecursionLength; i++)
	{
		// Recursion detected
		if (context->antiRecursion[i] == id) {
			count++;
		}
	}

	if (count > context->maxRecursion) {
		json_end(L, context, "Structure reached max recursion");
		return;
	}

	size_t pos = 0;

	if (context->antiRecursionEmpty > 0) {
		for (size_t i = 0; i < context->antiRecursionLength; i++) {
			if (context->antiRecursion[i] == 0) {
				context->antiRecursionEmpty--;
				context->antiRecursion[i] = id;
				pos = i;
				break;
			}
		}
	}
	else {
		pos = context->antiRecursionLength++;
		context->antiRecursion[pos] = id;
	}

	int objlen = 0;

	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {

		objlen++;

		lua_pop(L, 1);
	}

	// Object
	if (objlen == size && size <= 0) {
		json_append("[]", 2, L, context);
	}
	else if (objlen != size) {

		json_append("{", 1, L, context);

		lua_pushnil(L);
		while (lua_next(L, -2) != 0) {

			lua_pushvalue(L, -2);
			json_appendstring(L, context);
			lua_pop(L, 1);
			json_append(":", 1, L, context);
			json_appendvalue(L, context);
			json_append(",", 1, L, context);

			lua_pop(L, 1);
		}

		context->resultLength -= 1;
		json_append("}", 1, L, context);
	}
	// Array
	else {
		json_append("[", 1, L, context);

		for (int i = 1; i <= size; i++)
		{
			lua_rawgeti(L, -1, i);
			json_appendvalue(L, context);
			json_append(",", 1, L, context);
			lua_pop(L, 1);
		}

		context->resultLength -= 1;
		json_append("]", 1, L, context);
	}

	context->antiRecursion[pos] = 0;
	context->antiRecursionEmpty++;
}

int lua_jsonnull(lua_State *L) {

	if (jsonNull == -1) {
		lua_newtable(L);
		jsonNull = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_rawgeti(L, LUA_REGISTRYINDEX, jsonNull);

		size_t len;
		const char * rawid = luaL_tolstring(L, -1, &len);
		lua_pop(L, 1);
		jsonNullValue = table_crc32((const unsigned char*)rawid, len);
	}
	else {
		lua_rawgeti(L, LUA_REGISTRYINDEX, jsonNull);
	}

	return 1;
}

int lua_jsonempty(lua_State *L) {

	if (jsonEmpty == -1) {
		lua_newtable(L);
		jsonEmpty = luaL_ref(L, LUA_REGISTRYINDEX);
		lua_rawgeti(L, LUA_REGISTRYINDEX, jsonEmpty);

		size_t len;
		const char * rawid = luaL_tolstring(L, -1, &len);
		lua_pop(L, 1);
		jsonEmptyValue = table_crc32((const unsigned char*)rawid, len);
	}
	else {
		lua_rawgeti(L, LUA_REGISTRYINDEX, jsonEmpty);
	}

	return 1;
}

int lua_jsonencode(lua_State *L) {

	luaL_checktype(L, 1, LUA_TTABLE);
	size_t maxRecursion = luaL_optinteger(L, 2, 10);
	lua_jsonnull(L);
	lua_jsonempty(L);
	if (lua_gettop(L) > 1) {
		lua_pop(L, lua_gettop(L) - 1);
	}

	if (maxRecursion <= 0) {
		maxRecursion = 1;
	}
	else if (maxRecursion > 1000) {
		maxRecursion = 1000;
	}

	JsonContext jsonContext;
	memset(&jsonContext, 0, sizeof(JsonContext));
	jsonContext.maxRecursion = maxRecursion;

	jsonContext.result = (char*)calloc(JSONINITBUFFERSIZE, sizeof(char));
	jsonContext.resultSize = JSONINITBUFFERSIZE;

	lua_jsonencodetable(L, &jsonContext);
	lua_pop(L, 1);
	lua_pushlstring(L, jsonContext.result, jsonContext.resultLength);
	json_end(L, &jsonContext, NULL);

	return 1;
}

void lua_readskipwhitespace(JsonContext* context) {

	for (size_t i = context->readCursor; context->readCursor < context->readLength; context->readCursor++)
	{
		switch (context->read[i])
		{
		case 10:
		case 13:
			context->readLine++;
			context->readPos = 0;
			break;
		case 32:
		case 9:
			context->readPos++;
			break;
		default:
			return;
		}
	}
}

void lua_rewind(JsonContext* context) {
	context->readPos--;
	context->readCursor--;
}

char lua_read(lua_State *L, JsonContext* context) {

	if (context->readCursor >= context->readLength) {
		json_end(L, context, "Read end of json before it was complete");
		return 0;
	}

	context->readPos++;
	return context->read[context->readCursor++];
}

void json_unexpected(char c, lua_State *L, JsonContext* context) {
	char buf[100];
	sprintf(buf, "Unexpected character %c on line %u position %u", c, context->readLine, context->readPos);
	json_end(L, context, buf);
	return;
}

void lua_readchar(lua_State *L, JsonContext* context) {

	char hex[5] = { 0 };
	char result[2] = { 0 };

	for (size_t i = 0; i < 4; i++)
	{
		hex[i] = tolower(lua_read(L, context));

		if (!isxdigit(hex[i])) {
			json_unexpected(hex[i], L, context);
			return;
		}
	}

	int first;
	int second;
	sscanf(hex, "%02x%02x", &first, &second);

	result[0] = (char)first;
	result[1] = (char)second;

	json_append(result, 2, L, context);
}

void lua_readstring(lua_State *L, JsonContext* context) {

	context->resultLength = 0;
	char next = lua_read(L, context);
	if (next != '"') {
		json_unexpected(next, L, context);
		return;
	}

	bool isEscaping = false;
	while (true) {

		next = lua_read(L, context);

		if (isEscaping) {

			isEscaping = false;

			switch (next) {
			case '"':
				next = '"';
				break;
			case '/':
				next = '/';
				break;
			case '\\':
				next = '\\';
				break;
			case 'a':
				next = 0x07;
				break;
			case 'e':
				next = 0x1B;
				break;
			case 'b':
				next = 0x08;
				break;
			case 'v':
				next = 0x0B;
				break;
			case '0':
				next = 0x00;
				break;
			case 'f':
				next = 0x0C;
				break;
			case 'n':
				next = 0x0A;
				break;
			case 'r':
				next = 0x0D;
				break;
			case 't':
				next = 0x09;
				break;
			case 'u':
				lua_readchar(L, context);
				continue;
			default:
				break;
			}
		}
		else if (next == '\\') {
			isEscaping = true;
			continue;
		}
		else if (next == '"') {
			break;
		}

		json_append(&next, 1, L, context);
	}

	lua_pushlstring(L, context->result, context->resultLength);
}

static bool parse_sign(const char **const s)
{
	switch (**s) {
	case '-': ++*s; return false;
	case '+': ++*s; return true;
	default: return true;
	}
}

static double parse_digits(const char **const s, int *const count)
{
	double value = 0.0;
	int c = 0;
	while (isdigit(**s)) {
		value = value * 10.0 + (*(*s)++ - '0');
		++c;
	}
	if (count)
		*count = c;
	return value;
}

// https://codereview.stackexchange.com/questions/158519/c-program-to-convert-string-to-floating-point
double extended_atof(const char *s)
{
	/*skip white space*/
	while (isspace(*s))
		++s;

	const bool valuesign = parse_sign(&s); /* sign of the number */
	double value = parse_digits(&s, NULL);

	if (*s == '.') {
		int d;                  /* number of digits in fraction */
		++s;
		double fraction = parse_digits(&s, &d);
		while (d--)
			fraction /= 10.0;
		value += fraction;
	}

	if (!valuesign)
		value = -value;

	if (tolower(*s++) != 'e')
		return value;

	/* else, we have an exponent; parse its sign and value */
	const double exponentsign = parse_sign(&s) ? 10. : .1;
	int exponent = parse_digits(&s, NULL);
	while (exponent--)
		value *= exponentsign;

	return value;
}

void lua_readnumber(lua_State *L, JsonContext* context) {

	context->resultLength = 0;
	char next = lua_read(L, context);
	bool fract = false;
	bool exp = false;
	bool expplusminus = false;

	if (next == '-') {
		json_append(&next, 1, L, context);
		next = lua_read(L, context);
	}

	while (true) {

		next = tolower(next);

		if (next <= 32) {
			break;
		}
		else if (next == ',' || next == ']' || next == '}') {
			lua_rewind(context);
			break;
		}
		else if (next == '.' && !fract) {
			fract = true;
			json_append(&next, 1, L, context);
		}
		else if (next == 'e' && !exp) {
			exp = true;
			json_append(&next, 1, L, context);
		}
		else if ((next == '+' || next == '-') && exp && !expplusminus) {
			expplusminus = true;
			json_append(&next, 1, L, context);
		}
		else if (isdigit(next)) {
			json_append(&next, 1, L, context);
		}
		else {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
	}

	if (exp || fract) {
		lua_pushnumber(L, extended_atof(context->result));
	}
	else {
		lua_pushinteger(L, atoll(context->result));
	}
}

void lua_readvalue(lua_State *L, JsonContext* context) {

	char next = lua_read(L, context);

	if (next == '"') {
		lua_rewind(context);
		lua_readstring(L, context);
	}
	else if (next == '{' || next == '[') {
		lua_rewind(context);
		lua_readtable(L, context);
	}
	else if (next == '-' || isdigit(next)) {
		lua_rewind(context);
		lua_readnumber(L, context);
	}
	else if (tolower(next) == 't') {

		next = lua_read(L, context);
		if (tolower(next) != 'r') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'u') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'e') {
			json_unexpected(next, L, context);
			return;
		}

		lua_pushboolean(L, 1);
	}
	else if (tolower(next) == 'f') {

		next = lua_read(L, context);
		if (tolower(next) != 'a') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'l') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 's') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'e') {
			json_unexpected(next, L, context);
			return;
		}

		lua_pushboolean(L, 0);
	}
	else if (tolower(next) == 'n') {

		next = lua_read(L, context);
		if (tolower(next) != 'u') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'l') {
			json_unexpected(next, L, context);
			return;
		}

		next = lua_read(L, context);
		if (tolower(next) != 'l') {
			json_unexpected(next, L, context);
			return;
		}

		lua_pushnil(L);
	}
	else {
		json_unexpected(next, L, context);
	}
}

void lua_readtable(lua_State *L, JsonContext* context) {

	lua_readskipwhitespace(context);
	char next = lua_read(L, context);

	if (next == '{') {

		lua_newtable(L);
		lua_readskipwhitespace(context);
		next = lua_read(L, context);

		if (next == '}') {
			return;
		}
		else {

			while (true) {

				lua_rewind(context);
				lua_readstring(L, context);
				lua_readskipwhitespace(context);
				next = lua_read(L, context);

				if (next != ':') {
					json_unexpected(next, L, context);
				}

				lua_readskipwhitespace(context);
				lua_readvalue(L, context);
				lua_settable(L, -3);
				lua_readskipwhitespace(context);
				next = lua_read(L, context);

				if (next == ',') {
					lua_readskipwhitespace(context);
					next = lua_read(L, context);
					continue;
				}
				else if (next == '}') {
					break;
				}
				else {
					json_unexpected(next, L, context);
					return;
				}
			}
		}
	}
	else if (next == '[') {

		int cnt = 0;
		lua_readskipwhitespace(context);
		lua_newtable(L);

		next = lua_read(L, context);
		if (next == ']') {
			return;
		}
		else {
			lua_rewind(context);
		}

		while (true) {

			lua_readvalue(L, context);
			lua_rawseti(L, -2, ++cnt);

			lua_readskipwhitespace(context);
			next = lua_read(L, context);

			if (next == ',') {
				lua_readskipwhitespace(context);
				continue;
			}
			else if (next == ']') {
				break;
			}
			else {
				json_unexpected(next, L, context);
				return;
			}
		}
	}
	else {
		json_unexpected(next, L, context);
		return;
	}
}

int lua_jsondecode(lua_State *L) {

	size_t len;
	const char * str = luaL_checklstring(L, 1, &len);

	JsonContext jsonContext;
	memset(&jsonContext, 0, sizeof(JsonContext));
	jsonContext.read = str;
	jsonContext.readLength = len;
	lua_readtable(L, &jsonContext);

	lua_copy(L, -1, 1);
	lua_pop(L, lua_gettop(L) - 1);

	json_end(L, &jsonContext, NULL);

	return 1;
}