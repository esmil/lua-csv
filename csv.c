#include <stdlib.h>

#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>

enum classes {
	C_INV,   /* invalid characters */
	C_COMMA, /* , */
	C_DQUOT, /* " */
	C_LF,    /* \n */
	C_CR,    /* \r */
	C_ETC,   /* the rest */
	C_MAX
};

/*
 * This array maps the first 96 ASCII characters into character classes
 * The remaining characters should be mapped to C_ETC
 */
static const unsigned char ascii_class[64] = {
	C_INV,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_LF,    C_ETC,   C_ETC,   C_CR,    C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,

	C_ETC,   C_ETC,   C_DQUOT, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_COMMA, C_ETC,   C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
	C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
};

enum states {
	S_NLF,
	S_NCR,
	S_NCL,
	S_NST,
	S_STR,
	S_QST,
	S_QT1,
	S_QT2,
	S_MAX,
	S____ = S_MAX
};

static const unsigned char state_table[S_MAX][C_MAX] = {
/*             inv     ,      "      \n     \r    etc */
/* S_NLF */ { S____, S_NST, S_QT1, S_NLF, S_NCR, S_STR },
/* S_NCR */ { S____, S_NST, S_QT1, S_NCL, S_NCR, S_STR },
/* S_NCL */ { S____, S_NST, S_QT1, S_NLF, S_NCR, S_STR },
/* S_NST */ { S____, S_NST, S_QT1, S_NLF, S_NCR, S_STR },
/* S_STR */ { S____, S_NST, S____, S_NLF, S_NCR, S_STR },
/* S_QST */ { S____, S_QST, S_QT2, S_QST, S_QST, S_QST },
/* S_QT1 */ { S____, S_QST, S_QT2, S_QST, S_QST, S_QST },
/* S_QT2 */ { S____, S_NST, S_QST, S_NLF, S_NCR, S____ }
};

#define BUFLEN 512
struct context {
	unsigned char state;
	int index;
	int line;
};

static int
csv_new(lua_State *L)
{
	struct context *pc;

	pc = lua_newuserdata(L, sizeof(struct context));
	pc->state = S_NLF;
	pc->index = 1;
	pc->line = 1;

	/* set metatable */
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_setmetatable(L, -2);

	lua_createtable(L, 0, 0);
	lua_createtable(L, 0, 0);
	lua_rawseti(L, -2, 1);
	lua_setfenv(L, -2);

	return 1;
}

static int
csv_add(lua_State *L)
{
	struct context *pc;
	const char *str;
	size_t len;
	unsigned char state;
	int parts = 1;
	char buf[BUFLEN];
	char *p = buf;

	luaL_checktype(L, 1, LUA_TUSERDATA);
	luaL_checktype(L, 2, LUA_TSTRING);

	pc = lua_touserdata(L, 1);
	str = lua_tolstring(L, 2, &len);

	lua_settop(L, 2);
	lua_getfenv(L, 1);
	lua_rawgeti(L, -1, pc->line);

	state = pc->state;
	switch (state) {
	case S_STR:
	case S_QST:
		lua_rawgeti(L, -1, pc->index);
		parts = 2;
		break;

	case S____:
		lua_pushnil(L);
		lua_pushliteral(L, "Parse error");
		return 2;
	}

	for (; len > 0; len--) {
		unsigned char c = *str++;

		state = state_table[state][c < 64 ? ascii_class[c] : C_ETC];

		switch (state) {
		case S_STR:
		case S_QST:
			*p++ = c;
			if (p - buf == BUFLEN) {
				lua_pushlstring(L, buf, BUFLEN);
				parts++;
				p = buf;
			}
			break;

		case S_NST:
			lua_pushlstring(L, buf, p - buf);
			lua_concat(L, parts);
			lua_rawseti(L, -2, pc->index++);
			parts = 1;
			p = buf;
			break;

		case S_NLF:
		case S_NCR:
			lua_pushlstring(L, buf, p - buf);
			lua_concat(L, parts);
			lua_rawseti(L, -2, pc->index);
			parts = 1;
			p = buf;

			lua_rawseti(L, -2, pc->line++);
			pc->index = 1;

			lua_createtable(L, 0, 0);
			break;

		case S____:
			pc->state = state;
			lua_pushnil(L);
			lua_pushliteral(L, "Parse error");
			return 2;
		}
	}

	switch (state) {
	case S_STR:
	case S_QST:
		lua_pushlstring(L, buf, p - buf);
		lua_concat(L, parts);
		lua_rawseti(L, -2, pc->index);
	}
	lua_rawseti(L, -2, pc->line);

	pc->state = state;
	/* return true */
	lua_pushboolean(L, 1);
	return 1;
}

static int
csv_finish(lua_State *L)
{
	struct context *pc;

	luaL_checktype(L, 1, LUA_TUSERDATA);

	pc = lua_touserdata(L, 1);
	switch (pc->state) {
	case S_NLF:
	case S_NCR:
	case S_NCL:
	case S_NST:
		break;

	default:
		lua_pushnil(L);
		lua_pushliteral(L, "Parse error");
		return 2;
	}

	pc->state = S____;

	lua_settop(L, 1);
	lua_getfenv(L, 1);

	lua_pushnil(L);
	lua_rawseti(L, -2, pc->line);

	lua_pushnil(L);
	return 2;
}

LUALIB_API int
luaopen_csv_core(lua_State *L)
{
	lua_settop(L, 0);

	lua_createtable(L, 0, 2);

	lua_pushliteral(L, "mt");
	lua_createtable(L, 0, 2);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, 3);

	lua_pushliteral(L, "add");
	lua_pushcclosure(L, csv_add, 0);
	lua_rawset(L, 3);

	lua_pushliteral(L, "finish");
	lua_pushcclosure(L, csv_finish, 0);
	lua_rawset(L, 3);

	lua_pushliteral(L, "new");
	lua_pushvalue(L, 3);
	lua_pushcclosure(L, csv_new, 1);
	lua_rawset(L, 1);

	lua_rawset(L, 1);

	return 1;
}
