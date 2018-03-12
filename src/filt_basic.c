#include "fitz.h"

/* Pretend we have a filter that just copies data forever */

fz_stream *
fz_opencopy(fz_stream *chain)
{
	return fz_keepstream(chain);
}

/* Null filter copies a specified amount of data */

struct nullfilter
{
	fz_stream *chain;
	int remain;
};

static int
readnull(fz_stream *stm, unsigned char *buf, int len)
{
	struct nullfilter *state = stm->state;
	int amount = MIN(len, state->remain);
	int n = fz_read(state->chain, buf, amount);
	if (n < 0)
		return fz_rethrow(n, "read error in null filter");
	state->remain -= n;
	return n;
}

static void
closenull(fz_stream *stm)
{
	struct nullfilter *state = stm->state;
	fz_close(state->chain);
	fz_free(state);
}

fz_stream *
fz_opennull(fz_stream *chain, int len)
{
	struct nullfilter *state;

	state = fz_malloc(sizeof(struct nullfilter));
	state->chain = chain;
	state->remain = len;

	return fz_newstream(state, readnull, closenull);
}

/* ASCII Hex Decode */

typedef struct fz_ahxd_s fz_ahxd;

struct fz_ahxd_s
{
	fz_stream *chain;
	int eod;
};

static inline int iswhite(int a)
{
	switch (a) {
	case '\n': case '\r': case '\t': case ' ':
	case '\0': case '\f': case '\b': case 0177:
		return 1;
	}
	return 0;
}

static inline int ishex(int a)
{
	return (a >= 'A' && a <= 'F') ||
		(a >= 'a' && a <= 'f') ||
		(a >= '0' && a <= '9');
}

static inline int fromhex(int a)
{
	if (a >= 'A' && a <= 'F')
		return a - 'A' + 0xA;
	if (a >= 'a' && a <= 'f')
		return a - 'a' + 0xA;
	if (a >= '0' && a <= '9')
		return a - '0';
	return 0;
}

static int
readahxd(fz_stream *stm, unsigned char *buf, int len)
{
	fz_ahxd *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	int a, b, c, odd;

	odd = 0;

	while (p < ep)
	{
		if (state->eod)
			return p - buf;

		c = fz_readbyte(state->chain);
		if (c < 0)
			return p - buf;

		if (ishex(c))
		{
			if (!odd)
			{
				a = fromhex(c);
				odd = 1;
			}
			else
			{
				b = fromhex(c);
				*p++ = (a << 4) | b;
				odd = 0;
			}
		}
		else if (c == '>')
		{
			if (odd)
				*p++ = (a << 4);
			state->eod = 1;
		}
		else if (!iswhite(c))
		{
			return fz_throw("bad data in ahxd: '%c'", c);
		}
	}

	return p - buf;
}

static void
closeahxd(fz_stream *stm)
{
	fz_ahxd *state = stm->state;
	fz_close(state->chain);
	fz_free(state);
}

fz_stream *
fz_openahxd(fz_stream *chain)
{
	fz_ahxd *state;

	state = fz_malloc(sizeof(fz_ahxd));
	state->chain = chain;
	state->eod = 0;

	return fz_newstream(state, readahxd, closeahxd);
}

/* ASCII 85 Decode */

typedef struct fz_a85d_s fz_a85d;

struct fz_a85d_s
{
	fz_stream *chain;
	unsigned char bp[4];
	unsigned char *rp, *wp;
	int eod;
};

static int
reada85d(fz_stream *stm, unsigned char *buf, int len)
{
	fz_a85d *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	int count = 0;
	int word = 0;
	int c;

	while (state->rp < state->wp && p < ep)
		*p++ = *state->rp++;

	while (p < ep)
	{
		if (state->eod)
			return p - buf;

		c = fz_readbyte(state->chain);
		if (c < 0)
			return p - buf;

		if (c >= '!' && c <= 'u')
		{
			if (count == 4)
			{
				word = word * 85 + (c - '!');

				state->bp[0] = (word >> 24) & 0xff;
				state->bp[1] = (word >> 16) & 0xff;
				state->bp[2] = (word >> 8) & 0xff;
				state->bp[3] = (word) & 0xff;
				state->rp = state->bp;
				state->wp = state->bp + 4;

				word = 0;
				count = 0;
			}
			else
			{
				word = word * 85 + (c - '!');
				count ++;
			}
		}

		else if (c == 'z' && count == 0)
		{
			state->bp[0] = 0;
			state->bp[1] = 0;
			state->bp[2] = 0;
			state->bp[3] = 0;
			state->rp = state->bp;
			state->wp = state->bp + 4;
		}

		else if (c == '~')
		{
			c = fz_readbyte(state->chain);
			if (c != '>')
				return fz_throw("bad eod marker in a85d");

			switch (count) {
			case 0:
				break;
			case 1:
				return fz_throw("partial final byte in a85d");
			case 2:
				word = word * (85 * 85 * 85) + 0xffffff;
				state->bp[0] = word >> 24;
				state->rp = state->bp;
				state->wp = state->bp + 1;
				break;
			case 3:
				word = word * (85 * 85) + 0xffff;
				state->bp[0] = word >> 24;
				state->bp[1] = word >> 16;
				state->rp = state->bp;
				state->wp = state->bp + 2;
				break;
			case 4:
				word = word * 85 + 0xff;
				state->bp[0] = word >> 24;
				state->bp[1] = word >> 16;
				state->bp[2] = word >> 8;
				state->rp = state->bp;
				state->wp = state->bp + 3;
				break;
			}
			state->eod = 1;
		}

		else if (!iswhite(c))
		{
			return fz_throw("bad data in a85d: '%c'", c);
		}

		while (state->rp < state->wp && p < ep)
			*p++ = *state->rp++;
	}

	return p - buf;
}

static void
closea85d(fz_stream *stm)
{
	fz_a85d *state = stm->state;
	fz_close(state->chain);
	fz_free(state);
}

fz_stream *
fz_opena85d(fz_stream *chain)
{
	fz_a85d *state;

	state = fz_malloc(sizeof(fz_a85d));
	state->chain = chain;
	state->rp = state->bp;
	state->wp = state->bp;
	state->eod = 0;

	return fz_newstream(state, reada85d, closea85d);
}

/* Run Length Decode */

typedef struct fz_rld_s fz_rld;

struct fz_rld_s
{
	fz_stream *chain;
	int run, n, c;
};

static int
readrld(fz_stream *stm, unsigned char *buf, int len)
{
	fz_rld *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;

	while (p < ep)
	{
		if (state->run == 128)
			return p - buf;

		if (state->n == 0)
		{
			state->run = fz_readbyte(state->chain);
			if (state->run < 0)
				state->run = 128;
			if (state->run < 128)
				state->n = state->run + 1;
			if (state->run > 128)
			{
				state->n = 257 - state->run;
				state->c = fz_readbyte(state->chain);
				if (state->c < 0)
					return fz_throw("premature end of data in run length decode");
			}
		}

		if (state->run < 128)
		{
			while (p < ep && state->n)
			{
				int c = fz_readbyte(state->chain);
				if (c < 0)
					return fz_throw("premature end of data in run length decode");
				*p++ = c;
				state->n--;
			}
		}

		if (state->run > 128)
		{
			while (p < ep && state->n)
			{
				*p++ = state->c;
				state->n--;
			}
		}
	}

	return p - buf;
}

static void
closerld(fz_stream *stm)
{
	fz_rld *state = stm->state;
	fz_close(state->chain);
	fz_free(state);
}

fz_stream *
fz_openrld(fz_stream *chain)
{
	fz_rld *state;

	state = fz_malloc(sizeof(fz_rld));
	state->chain = chain;
	state->run = 0;
	state->n = 0;
	state->c = 0;

	return fz_newstream(state, readrld, closerld);
}

/* AES Filter */

