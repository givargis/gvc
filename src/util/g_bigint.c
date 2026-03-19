// Copyright (c) Tony Givargis, 2024-2026

#include "g_bigint.h"

#define MAX_PARTS 16384

#define IS_ONE(a)      ( (1 == (a)->width) && (1 == (a)->parts[0]) )
#define IS_ZERO(a)     ( 0 == (a)->width )
#define IS_NEGATIVE(a) ( (a)->sign )

struct g_bigint {
	int sign;
	int width;
	uint64_t *parts;
};

static uint64_t C_[17]  = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
};

static struct g_bigint C__[17] = {
	{ 0,0, NULL    }, { 0,1, &C_[ 1] }, { 0,1, &C_[ 2] }, { 0,1, &C_[ 3] },
	{ 0,1, &C_[ 4] }, { 0,1, &C_[ 5] }, { 0,1, &C_[ 6] }, { 0,1, &C_[ 7] },
	{ 0,1, &C_[ 8] }, { 0,1, &C_[ 9] }, { 0,1, &C_[10] }, { 0,1, &C_[11] },
	{ 0,1, &C_[12] }, { 0,1, &C_[13] }, { 0,1, &C_[14] }, { 0,1, &C_[15] },
	{ 0,1, &C_[16] }
};

const g_bigint_t G_BIGINT_CONST[17] = {
	&C__[ 0], &C__[ 1], &C__[ 2], &C__[ 3], &C__[ 4],
	&C__[ 5], &C__[ 6], &C__[ 7], &C__[ 8], &C__[ 9],
	&C__[10], &C__[11], &C__[12], &C__[13], &C__[14],
	&C__[15], &C__[16]
};

static void
destroy(struct g_bigint *z)
{
	if (z) {
		g_free(z->parts);
		memset(z, 0, sizeof (struct g_bigint));
		g_free(z);
	}
}

static struct g_bigint *
create(int width)
{
	struct g_bigint *z;

	if (MAX_PARTS < width) {
		G_TRACE("integer exceeds maximum limit");
		return NULL;
	}
	if (!(z = g_malloc(sizeof (struct g_bigint)))) {
		G_TRACE("^");
		return NULL;
	}
	memset(z, 0, sizeof (struct g_bigint));
	if ((z->width = width)) {
		if (!(z->parts = g_malloc(z->width * sizeof (z->parts[0])))) {
			destroy(z);
			G_TRACE("^");
			return NULL;
		}
	}
	return z;
}

static struct g_bigint *
clone(const struct g_bigint *a)
{
	struct g_bigint *z;

	if (!(z = create(a->width))) {
		G_TRACE("^");
		return NULL;
	}
	z->sign = a->sign;
	memcpy(z->parts, a->parts, z->width * sizeof (z->parts[0]));
	return z;
}

static void
normalize(struct g_bigint *z)
{
	while (z->width && !z->parts[z->width - 1]) {
		--z->width;
	}
	if (!z->width) {
		g_free(z->parts);
		z->parts = NULL;
		z->sign = 0;
	}
}

static int
cmp(const struct g_bigint *a, const struct g_bigint *b)
{
	if (a->sign > b->sign) {
		return -1;
	}
	if (a->sign < b->sign) {
		return +1;
	}
	if (a->width > b->width) {
		return a->sign ? -1 : +1;
	}
	if (a->width < b->width) {
		return a->sign ? +1 : -1;
	}
	for (int i=a->width-1; i>=0; --i) {
		if (a->parts[i] > b->parts[i]) {
			return a->sign ? -1 : +1;
		}
		if (a->parts[i] < b->parts[i]) {
			return a->sign ? +1 : -1;
		}
	}
	return 0;
}

static struct g_bigint *
uadd(const struct g_bigint *a, const struct g_bigint *b)
{
	struct g_bigint *z;
	int c;

	c = 0;
	if (!(z = create(G_MAX(a->width, b->width) + 1))) {
		G_TRACE("^");
		return NULL;
	}
	for (int i=0; i<z->width; ++i) {
		uint64_t a_ = (i < a->width) ? a->parts[i] : 0;
		uint64_t b_ = (i < b->width) ? b->parts[i] : 0;
		uint64_t z_;
		z_  = a_ + b_ + (uint64_t)c;
		c = (z_ < a_) || (c && (z_ == a_));
		z->parts[i] = z_;
	}
	normalize(z);
	return z;
}

static struct g_bigint *
usub(const struct g_bigint *a, const struct g_bigint *b)
{
	struct g_bigint *z;
	int c;

	c = 0;
	if (!(z = create(a->width))) {
		G_TRACE("^");
		return NULL;
	}
	for (int i=0; i<z->width; ++i) {
		uint64_t a_ = a->parts[i];
		uint64_t b_ = (i < b->width) ? b->parts[i] : 0;
		uint64_t z_;
		z_ = a_ - b_ - (uint64_t)c;
		c = (a_ < b_) || (c && (a_ == b_));
		z->parts[i] = z_;
	}
	normalize(z);
	return z;
}

static struct g_bigint *
add(const struct g_bigint *a, const struct g_bigint *b)
{
	struct g_bigint *z;
	int d;

	if (a->sign == b->sign) {
		if (!(z = uadd(a, b))) {
			G_TRACE("^");
			return NULL;
		}
		z->sign = a->sign;
	}
	else {
		if (!(d = cmp(a, b))) {
			if (!(z = create(0))) {
				G_TRACE("^");
				return NULL;
			}
		}
		else if (0 < d) {
			if (!(z = usub(a, b))) {
				G_TRACE("^");
				return NULL;
			}
			z->sign = a->sign;
		}
		else {
			if (!(z = usub(b, a))) {
				G_TRACE("^");
				return NULL;
			}
			z->sign = b->sign;
		}
	}
	return z;
}

static struct g_bigint *
sub(const struct g_bigint *a, const struct g_bigint *b_)
{
	struct g_bigint *z, b;

	b = (*b_);
	b.sign = b_->sign ? 0 : 1;
	if (!(z = add(a, &b))) {
		G_TRACE("^");
		return NULL;
	}
	return z;
}

static void
mul128(uint64_t *h, uint64_t *l, uint64_t a, uint64_t b)
{
#ifdef __SIZEOF_INT128__
	__uint128_t t = (__uint128_t)a * (__uint128_t)b;
	(*h) = (uint64_t)(t >> 64);
	(*l) = (uint64_t)t;
#else
	uint64_t ah = a >> 32;
	uint64_t al = a & 0xffffffff;
	uint64_t bh = b >> 32;
	uint64_t bl = b & 0xffffffff;
	uint64_t t1 = bl * al;
	uint64_t t2 = bl * ah;
	uint64_t t3 = bh * al;
	uint64_t t4 = bh * ah;
	t2 += t1 >> 32;
	t3 += t2;
	if (t3 < t2) {
		t4 += 1LU << 32;
	}
	t4 += t3 >> 32;
	(*h) = t4;
	(*l) = (t3 << 32) | (t1 & 0xffffffff);
#endif
}

static struct g_bigint *
mul(const struct g_bigint *a, const struct g_bigint *b)
{
	struct g_bigint *z;
	uint64_t h, l;

	if (!(z = create(a->width + b->width))) {
		G_TRACE("^");
		return NULL;
	}
	memset(z->parts, 0, z->width * sizeof (z->parts[0]));
	for (int i=0; i<a->width; ++i) {
		for (int j=0; j<b->width; ++j) {
			mul128(&h, &l, a->parts[i], b->parts[j]);
			z->parts[i + j] += l;
			if (z->parts[i + j] < l) {
				int k = i + j + 1;
				while(!(++z->parts[k++]));
			}
			z->parts[i + j + 1] += h;
			if (z->parts[i + j + 1] < h) {
				int k = i + j + 2;
				while(!(++z->parts[k++]));
			}
		}
	}
	z->sign = a->sign ^ b->sign;
	normalize(z);
	return z;
}

static int
slow_divmod(const struct g_bigint *a,
	    const struct g_bigint *b,
	    struct g_bigint **q,
	    struct g_bigint **r)
{
	struct g_bigint *q_, *r_, *b2, *q2;

	if (0 > cmp(a, b)) {
		if (!((*q) = create(0)) || !((*r) = clone(a))) {
			destroy(*q);
			destroy(*r);
			G_TRACE("^");
			return -1;
		}
	}
	else {
		q_ = r_ = NULL;
		if (!(b2 = mul(b, G_BIGINT_CONST[2])) ||
		    slow_divmod(a, b2, &q_, &r_) ||
		    !(q2 = mul(q_, G_BIGINT_CONST[2]))) {
			destroy(q_);
			destroy(r_);
			destroy(b2);
			G_TRACE("^");
			return -1;
		}
		destroy(b2);
		if (0 > cmp(r_, b)) {
			(*q) = q2;
			(*r) = r_;
			destroy(q_);
		}
		else {
			(*q) = add(q2, G_BIGINT_CONST[1]);
			(*r) = sub(r_, b);
			destroy(q_);
			destroy(r_);
			destroy(q2);
			if (!(*q) || !(*r)) {
				G_TRACE("^");
				return -1;
			}
		}
	}
	normalize(*q);
	normalize(*r);
	return 0;
}

static int
divmod(const struct g_bigint *a_,
       const struct g_bigint *b_,
       struct g_bigint **q,
       struct g_bigint **r)
{
	struct g_bigint a, b;

	if (IS_ZERO(b_)) {
		G_TRACE("divide by zero");
		return -1;
	}
	a = (*a_);
	b = (*b_);
	a.sign = 0;
	b.sign = 0;
	if (slow_divmod(&a, &b, q, r)) {
		G_TRACE("^");
		return -1;
	}
	if (IS_NEGATIVE(a_) ^ IS_NEGATIVE(b_)) {
		(*q)->sign = 1;
	}
	if (IS_NEGATIVE(a_)) {
		(*r)->sign = 1;
	}
	return 0;
}

static int
count_bits(const struct g_bigint *a)
{
	int i;

	if (0 > (i = a->width - 1)) {
		return 1;
	}
	return i * 64 + (64 - g_clz(a->parts[i]));
}

static int
count_digits(const struct g_bigint *a)
{
	return (int)ceil(count_bits(a) * log(2) / log(10));
}

static int
hex2int(int c)
{
	c = tolower(c);
	if (('0' <= c) && ('9' >= c)) {
		return c - '0';
	}
	if (('a' <= c) && ('f' >= c)) {
		return c - 'a' + 10;
	}
	return -1;
}

static int
bin2int(int c)
{
	if (('0' <= c) && ('1' >= c)) {
		return c - '0';
	}
	return -1;
}

static int
dec2int(int c)
{
	if (('0' <= c) && ('9' >= c)) {
		return c - '0';
	}
	return -1;
}

static struct g_bigint *
convert_int(int64_t v)
{
	struct g_bigint *z;

	if (!(z = create(1))) {
		G_TRACE("^");
		return NULL;
	}
	z->parts[0] = (0 > v) ? ~(uint64_t)v + 1 : (uint64_t)v;
	z->sign = (0 > v) ? 1 : 0;
	normalize(z);
	return z;
}

static struct g_bigint *
convert_real(double r)
{
	struct g_bigint *z;
	uint64_t mantissa;
	int i, exp, sign;

	// handle corner cases, simplifying the main conversion process

	if (g_isnan(r)) {
		G_TRACE("invalid NaN real value");
		return NULL;
	}
	else if (g_isinf(r)) {
		G_TRACE("invalid INF real value");
		return NULL;
	}

	// handle small values to avoid IEEE denormalization cases

	if ((LONG_MIN < r) && (LONG_MAX > r)) {
		if (!(z = convert_int(g_lround(r)))) {
			G_TRACE("^");
			return NULL;
		}
		return z;
	}

	// capture the sign

	sign = 0;
	if (0.0 > r) {
		sign = 1;
		r = fabs(r);
	}

	// exponent and mantissa, adding the implicit 53rd bit to mantissa

	r = frexp(r, &exp);
	memcpy(&mantissa, &r, sizeof (mantissa));
	mantissa &= 0xfffffffffffff;
	mantissa |= 0x10000000000000;

	// sanity check

	assert( (64 < exp) && (1024 > exp) );

	// convert

	if (!(z = create(G_DUP(exp, 64)))) {
		G_TRACE("^");
		return NULL;
	}

	// trailing zero parts

	i = 0;
	exp -= 53; // this many significant bits
	while (64 <= exp) {
		exp -= 64;
		z->parts[i++] = 0;
	}
	exp += 53;

	// significant bits

	z->parts[i++] = mantissa << (exp - 53);
	if (i < z->width) {
		z->parts[i++] = mantissa >> (117 - exp);
	}
	z->sign = sign;
	normalize(z);
	return z;
}

static struct g_bigint *
convert_string(const char *s)
{
	struct g_bigint *a, *b, *z;
	int (*p2v)(int);
	const char *e, *s_;
	double r;
	int v, m;

	m = 10;
	p2v = dec2int;
	if (('0' == s[0]) && (('x' == s[1]) || ('X' == s[1]))) {
		m = 16;
		p2v = hex2int;
		s += 2;
	}
	else if (('0' == s[0]) && (('b' == s[1]) || ('B' == s[1]))) {
		m = 2;
		p2v = bin2int;
		s += 2;
	}
	else {
		errno = 0;
		r = strtod(s, (char **)&e);
		if ((EINVAL == errno) || (ERANGE == errno)) {
			G_TRACE("invalid real value");
			return NULL;
		}
		s_ = s;
		while (s < e) {
			if (('.' == (*s)) || ('e' == (*s)) || ('E' == (*s))) {
				return convert_real(r);
			}
			++s;
		}
		s = s_;
	}
	if (0 > (v = p2v((unsigned char)(*s)))) {
		G_TRACE("invalid integer value");
		return NULL;
	}
	if (!(z = convert_int(0))) {
		G_TRACE("^");
		return NULL;
	}
	while (*s) {
		if (0 > (v = p2v((unsigned char)(*s)))) {
			break;
		}
		if (!(a = mul(z, G_BIGINT_CONST[m])) ||
		    !(b = add(a, G_BIGINT_CONST[v]))) {
			destroy(a);
			destroy(z);
			G_TRACE("^");
			return NULL;
		}
		destroy(a);
		destroy(z);
		z = b;
		++s;
	}
	return z;
}

static char *
print(const struct g_bigint *a)
{
	static const uint64_t M_[] = { 1000000000000000000LU };
	const struct g_bigint M = { 0, 1, (uint64_t *)M_ };
	struct g_bigint *q, *r, *q_;
	uint64_t *stack;
	size_t i, len;
	char *buf;

	len = count_digits(a) + 2;
	if (!(buf = g_malloc(len)) ||
	    !(stack = g_malloc(G_DUP(len, 18) * sizeof (stack[0])))) {
		g_free(buf);
		G_TRACE("^");
		return NULL;
	}
	if (!(q = clone(a))) {
		g_free(buf);
		g_free(stack);
		G_TRACE("^");
		return NULL;
	}
	i = 0;
	while (!IS_ZERO(q)) {
		if (divmod(q, &M, &q_, &r)) {
			destroy(q);
			g_free(buf);
			g_free(stack);
			G_TRACE("^");
			return NULL;
		}
		stack[i++] = r->width ? r->parts[0] : 0;
		destroy(r);
		destroy(q);
		q = q_;
	}
	destroy(q);
	g_sprintf(buf, len, "0");
	if (i) {
		g_sprintf(buf,
			  len,
			  "%s%lu",
			  IS_NEGATIVE(a) ? "-" : "",
			  (unsigned long)stack[--i]);
		while (i) {
			g_sprintf(buf + strlen(buf),
				  len - strlen(buf),
				  "%018lu",
				  (unsigned long)stack[--i]);
		}
	}
	g_free(stack);
	return buf;
}

void
g_bigint_free(g_bigint_t z)
{
	destroy(z);
}

g_bigint_t
g_bigint_int(int64_t v)
{
	return convert_int(v);
}

g_bigint_t
g_bigint_real(double r)
{
	return convert_real(r);
}

g_bigint_t
g_bigint_string(const char *s)
{
	assert( s && (*s) );

	return convert_string(s);
}

char *
g_bigint_print(g_bigint_t a)
{
	return print(a);
}

g_bigint_t
g_bigint_add(g_bigint_t a, g_bigint_t b)
{
	return add(a, b);
}

g_bigint_t
g_bigint_sub(g_bigint_t a, g_bigint_t b)
{
	return sub(a, b);
}

g_bigint_t
g_bigint_mul(g_bigint_t a, g_bigint_t b)
{
	return mul(a, b);
}

g_bigint_t
g_bigint_div(g_bigint_t a, g_bigint_t b)
{
	struct g_bigint *q, *r;

	if (divmod(a, b, &q, &r)) {
		G_TRACE("^");
		return NULL;
	}
	destroy(r);
	return q;
}

g_bigint_t
g_bigint_mod(g_bigint_t a, g_bigint_t b)
{
	struct g_bigint *q, *r;

	if (divmod(a, b, &q, &r)) {
		G_TRACE("^");
		return NULL;
	}
	destroy(q);
	return r;
}

int
g_bigint_divmod(g_bigint_t a, g_bigint_t b, g_bigint_t *r, g_bigint_t *q)
{
	return divmod(a, b, r, q);
}

int
g_bigint_cmp(g_bigint_t a, g_bigint_t b)
{
	return cmp(a, b);
}

int
g_bigint_bits(g_bigint_t a)
{
	return count_bits(a);
}

int
g_bigint_digits(g_bigint_t a)
{
	return count_digits(a);
}

int
g_bigint_is_zero(g_bigint_t a)
{
	return IS_ZERO(a);
}

int
g_bigint_is_one(g_bigint_t a)
{
	return IS_ONE(a);
}

int
g_bigint_is_negative(g_bigint_t a)
{
	return IS_NEGATIVE(a);
}
