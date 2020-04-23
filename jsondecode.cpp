#include "jsondecode.h"
#include "jsonutil.h"

int json_lua_objectiterator(lua_State *L, int status, lua_KContext ctx);
int json_lua_arrayiterator(lua_State *L, int status, lua_KContext ctx);

int json_lua_arrayiterator(lua_State *L, int status, lua_KContext ctx) {

	JsonContext* context = lua_tojson(L, 1);

	json_advancewhitespace(L, context);
	char next = json_readnext(L, context);

	if (next == ',') {
		json_advancewhitespace(L, context);
		next = json_readnext(L, context);
	}
	else if (next == ']') {
		lua_pop(L, 1);
		unsigned int raw = json_popfromantirecursion(context);
		if (raw == 0) {	
			json_bail(L, context, NULL);
			lua_settop(L, 0); 
			return -1;
		}
		
		lua_pushnil(L);
		lua_pushnil(L);

		lua_yieldk(L, 2, 0, (lua_KFunction)raw);
		return 0;
	}

	int count = lua_tointeger(L, -1)+1;
	lua_pop(L, 1);
	lua_pushinteger(L, count);
	
	if (next == '{') {

		lua_pushinteger(L, count);
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "object");
		lua_settable(L, -3);

		json_addtoantirecursion((unsigned int)&json_lua_arrayiterator, context);
		lua_yieldk(L, 2, ctx, json_lua_objectiterator);
	}
	else if (next == '[') {

		lua_pushinteger(L, 0);
		lua_pushinteger(L, count);
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "array");
		lua_settable(L, -3);

		json_addtoantirecursion((unsigned int)&json_lua_arrayiterator, context);
		lua_yieldk(L, 2, ctx, json_lua_arrayiterator);
	}
	else {

		json_stepback(context);
		lua_pushinteger(L, count);
		json_decodevalue(L, context);

		lua_yieldk(L, 2, ctx, json_lua_arrayiterator);
	}

	return 0;
}

int json_lua_objectiterator(lua_State *L, int status, lua_KContext ctx) {

	JsonContext* context = lua_tojson(L, 1);
	json_advancewhitespace(L, context);
	char next = json_readnext(L, context);
	
	if (next == ',') {
		json_advancewhitespace(L, context);
		next = json_readnext(L, context);
	}
	else if (next == '}') {

		unsigned int raw = json_popfromantirecursion(context);
		if (raw == 0) {
			json_bail(L, context, NULL);
			lua_settop(L, 0);
			return -1;
		}

		lua_pushnil(L);
		lua_pushnil(L);

		lua_yieldk(L, 2, 0, (lua_KFunction)raw);
		return 0;
	}

	json_stepback(context);
	json_decodestring(L, context);

	json_advancewhitespace(L, context);
	next = json_readnext(L, context);

	if (next != ':') {
		json_unexpected(next, L, context);
		return 0;
	}

	json_advancewhitespace(L, context);
	next = json_readnext(L, context);

	if (next == '{') {

		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "object");
		lua_settable(L, -3);

		json_addtoantirecursion((unsigned int)&json_lua_objectiterator, context);
		lua_yieldk(L, 2, ctx, json_lua_objectiterator);
	}
	else if (next == '[') {

		lua_pushinteger(L, 0);
		lua_pushvalue(L, -2);
		lua_remove(L, -3);
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "array");
		lua_settable(L, -3);

		json_addtoantirecursion((unsigned int)&json_lua_objectiterator, context);
		lua_yieldk(L, 2, ctx, json_lua_arrayiterator);
	}
	else {

		json_stepback(context);
		json_decodevalue(L, context);
		lua_yieldk(L, 2, ctx, json_lua_objectiterator);
	}

	json_bail(L, context, NULL);

	return 0;
}

int json_lua_coroutineiterator(lua_State *L, int status, lua_KContext ctx) {

	JsonContext* context = lua_tojson(L, 1);

	json_advancewhitespace(L, context);

	char next = json_readnext(L, context);

	if (next == '{') {

		lua_pushstring(L, "root");
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "object");
		lua_settable(L, -3);

		lua_yieldk(L, 2, ctx, json_lua_objectiterator);
	}
	else if (next == '[') {

		lua_pushinteger(L, 0);
		lua_pushstring(L, "root");
		lua_createtable(L, 0, 1);
		lua_pushstring(L, "type");
		lua_pushstring(L, "array");
		lua_settable(L, -3);

		lua_yieldk(L, 2, ctx, json_lua_arrayiterator);
	}
	else {
		json_stepback(context);
		lua_pushstring(L, "root");
		json_decodevalue(L, context);
	}

	lua_yield(L, 2);

	return 0;
}

void json_advancewhitespace(lua_State *L, JsonContext* context) {

	char next = json_readnext(L, context);
	while (true) {

		switch (next)
		{
		case 10:
		case 13:
			context->readLine++;
			context->readPosition = 0;
			break;
		case 32:
		case 9:
			context->readPosition++;
			break;
		default:
			json_stepback(context);
			return;
		}

		next = json_readnext(L, context);
	}
}

char json_readnext(lua_State *L, JsonContext* context) {

	if (context->prevFileChar[0]) {
		context->prevFileChar[0] = 0;
		context->readPosition++;
		return context->prevFileChar[1];
	}

	if (context->refReadFunction != LUA_REFNIL && (!context->readFileBuffer || context->readCursor >= context->readSize)) {

		if (!context->readFileBuffer) {
			context->readFileBuffer = (char*)gff_malloc(JSONFILEREADBUFFERSIZE);
			if (!context->readFileBuffer) {

				lua_gc(L, LUA_GCCOLLECT, 0);
				context->readFileBuffer = (char*)gff_malloc(JSONFILEREADBUFFERSIZE);

				if (!context->readFileBuffer) {
					json_bail(L, context, "Out of memory");
				}
			}
			context->readFileBufferSize = JSONFILEREADBUFFERSIZE;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, context->refReadFunction);

		if (lua_pcall(L, 0, 1, NULL)) {

			const char * err = lua_tostring(L, -1);
			lua_pop(L, 1);

			if (!err) {
				err = "err";
			}

			json_bail(L, context, err);
		}

		size_t len;
		const char * data = lua_tolstring(L, -1, &len);

		if (len > 0 && data) {

			if (len >= context->readFileBufferSize) {

				void * temp = gff_realloc(context->readFileBuffer, len + 1);

				if (!temp) {
					lua_gc(L, LUA_GCCOLLECT, 0);
					temp = gff_realloc(context->readFileBuffer, len + 1);

					if (!temp) {
						json_bail(L, context, "Out of memory");
					}
				}

				context->readFileBuffer = (char*)temp;
				context->readFileBufferSize = len + 1;
			}

			memcpy(context->readFileBuffer, data, len);
			lua_pop(L, 1);

			context->readCursor = 0;
			context->read = context->readFileBuffer;
			context->readSize = len;
		}
	}
	else if (context->readFile && (!context->readFileBuffer || context->readCursor >= JSONFILEREADBUFFERSIZE)) {

		if (!context->readFileBuffer) {
			context->readFileBuffer = (char*)gff_malloc(JSONFILEREADBUFFERSIZE);

			if (!context->readFileBuffer) {

				lua_gc(L, LUA_GCCOLLECT, 0);

				context->readFileBuffer = (char*)gff_malloc(JSONFILEREADBUFFERSIZE);

				if (!context->readFileBuffer) {
					json_bail(L, context, "Out of memory");
					return 0;
				}
			}
		}

		context->readSize = fread(context->readFileBuffer, sizeof(char), JSONFILEREADBUFFERSIZE, context->readFile);

		context->readCursor = 0;
		context->read = context->readFileBuffer;
	}

	if (context->readCursor >= context->readSize) {
		json_bail(L, context, "Read end of json before it was complete");
		return 0;
	}

	context->prevFileChar[1] = context->read[context->readCursor++];
	context->readPosition++;
	return context->prevFileChar[1];
}

void json_stepback(JsonContext* context) {

	context->readPosition--;
	context->prevFileChar[0] = 1;
}

void json_unexpected(char c, lua_State *L, JsonContext* context) {
	char buf[100];
	sprintf(buf, "Unexpected character %c on line %u position %u", c, context->readLine, context->readPosition);
	json_bail(L, context, buf);
	return;
}

void json_decodecharacter(lua_State *L, JsonContext* context) {

	char hex[5] = { 0 };
	char result[2] = { 0 };

	for (size_t i = 0; i < 4; i++)
	{
		hex[i] = tolower(json_readnext(L, context));

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

	if (result[0] && result[1]) {
		json_append(result, 2, L, context);
	}
	else {
		json_append(&result[1], 1, L, context);
	}
}

void json_decodestring(lua_State *L, JsonContext* context) {

	context->bufferLength = 0;
	char next = json_readnext(L, context);
	if (next != '"') {
		json_unexpected(next, L, context);
		return;
	}

	bool isEscaping = false;
	while (true) {

		next = json_readnext(L, context);

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
				json_decodecharacter(L, context);
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

	lua_pushlstring(L, context->buffer, context->bufferLength);
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

void json_decodenumber(lua_State *L, JsonContext* context) {

	context->bufferLength = 0;
	char next = json_readnext(L, context);
	bool fract = false;
	bool exp = false;
	bool expplusminus = false;

	if (next == '-') {
		json_append(&next, 1, L, context);
		next = json_readnext(L, context);
	}

	while (true) {

		next = tolower(next);

		if (next <= 32) {
			break;
		}
		else if (next == ',' || next == ']' || next == '}') {
			json_stepback(context);
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

		next = json_readnext(L, context);
	}

	if (exp || fract) {
		lua_pushnumber(L, extended_atof(context->buffer));
	}
	else {
		lua_pushinteger(L, atoll(context->buffer));
	}
}

void json_decodevalue(lua_State *L, JsonContext* context) {

	char next = json_readnext(L, context);

	if (next == '"') {
		json_stepback(context);
		json_decodestring(L, context);
	}
	else if (next == '{' || next == '[') {
		json_stepback(context);
		json_decodetable(L, context);
	}
	else if (next == '-' || isdigit(next)) {
		json_stepback(context);
		json_decodenumber(L, context);
	}
	else if (tolower(next) == 't') {

		next = json_readnext(L, context);
		if (tolower(next) != 'r') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 'u') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 'e') {
			json_unexpected(next, L, context);
			return;
		}

		lua_pushboolean(L, 1);
	}
	else if (tolower(next) == 'f') {

		next = json_readnext(L, context);
		if (tolower(next) != 'a') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 'l') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 's') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 'e') {
			json_unexpected(next, L, context);
			return;
		}

		lua_pushboolean(L, 0);
	}
	else if (tolower(next) == 'n') {

		next = json_readnext(L, context);
		if (tolower(next) != 'u') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
		if (tolower(next) != 'l') {
			json_unexpected(next, L, context);
			return;
		}

		next = json_readnext(L, context);
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

void json_decodetable(lua_State *L, JsonContext* C) {

	json_advancewhitespace(L, C);
	char next = json_readnext(L, C);

	if (next == '{') {

		lua_newtable(L);
		json_advancewhitespace(L, C);
		next = json_readnext(L, C);

		if (next == '}') {
			return;
		}
		else {

			while (true) {

				json_stepback(C);

				next = json_readnext(L, C);

				if (next == '}') {
					return;
				}
				else {
					json_stepback(C);
				}

				json_decodestring(L, C);
				json_advancewhitespace(L, C);
				next = json_readnext(L, C);

				if (next != ':') {
					json_unexpected(next, L, C);
				}

				json_advancewhitespace(L, C);
				json_decodevalue(L, C);
				lua_settable(L, -3);
				json_advancewhitespace(L, C);
				next = json_readnext(L, C);

				if (next == ',') {
					json_advancewhitespace(L, C);
					next = json_readnext(L, C);
					continue;
				}
				else if (next == '}') {
					break;
				}
				else {
					json_unexpected(next, L, C);
					return;
				}
			}
		}
	}
	else if (next == '[') {

		int cnt = 0;
		json_advancewhitespace(L, C);
		lua_newtable(L);

		next = json_readnext(L, C);
		if (next == ']') {
			return;
		}
		else {
			json_stepback(C);
		}

		while (true) {

			next = json_readnext(L, C);

			if (next == ']') {
				return;
			}
			else {
				json_stepback(C);
			}

			json_decodevalue(L, C);
			lua_rawseti(L, -2, ++cnt);

			json_advancewhitespace(L, C);
			next = json_readnext(L, C);

			if (next == ',') {
				json_advancewhitespace(L, C);
				continue;
			}
			else if (next == ']') {
				break;
			}
			else {
				json_unexpected(next, L, C);
				return;
			}
		}
	}
	else {
		json_unexpected(next, L, C);
		return;
	}
}