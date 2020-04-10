#include "jsondecode.h"
#include "jsonutil.h"

void json_advancewhitespace(JsonContext* context) {

	if (context->readFile) {
		
		char byte;

		while (fread(&byte, sizeof(char), 1, context->readFile) == 1) {
			switch (byte)
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
		}
	}
	else {
		for (size_t i = context->readCursor; context->readCursor < context->readSize; context->readCursor++)
		{
			switch (context->read[i])
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
				return;
			}
		}
	}
}

char json_readnext(lua_State *L, JsonContext* context) {

	if (context->readFile) {

		char byte;

		if (fread(&byte, sizeof(char), 1, context->readFile) == 1) {
			return byte;
		}
		else {
			json_bail(L, context, "Read end of json before it was complete");
			return 0;
		}
	}
	else {

		if (context->readCursor >= context->readSize) {
			json_bail(L, context, "Read end of json before it was complete");
			return 0;
		}

		context->readPosition++;
		return context->read[context->readCursor++];
	}
}

void json_stepback(JsonContext* context) {
	
	context->readPosition--;

	if (context->readFile) {
		fseek(context->readFile, -1, SEEK_CUR);
	}
	else {
		context->readCursor--;
	}
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

	json_append(result, 2, L, context);
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

	json_advancewhitespace(C);
	char next = json_readnext(L, C);

	if (next == '{') {

		lua_newtable(L);
		json_advancewhitespace(C);
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
				json_advancewhitespace(C);
				next = json_readnext(L, C);

				if (next != ':') {
					json_unexpected(next, L, C);
				}

				json_advancewhitespace(C);
				json_decodevalue(L, C);
				lua_settable(L, -3);
				json_advancewhitespace(C);
				next = json_readnext(L, C);

				if (next == ',') {
					json_advancewhitespace(C);
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
		json_advancewhitespace(C);
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

			json_advancewhitespace(C);
			next = json_readnext(L, C);

			if (next == ',') {
				json_advancewhitespace(C);
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