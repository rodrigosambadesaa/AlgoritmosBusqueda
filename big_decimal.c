#include "big_decimal.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static char *bd_strdup(const char *s)
{
    size_t n;
    char *copy;

    if (s == NULL)
    {
        return NULL;
    }

    n = strlen(s) + 1U;
    copy = (char *)malloc(n);
    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, s, n);
    return copy;
}

static int is_all_digits(const char *s)
{
    size_t i;

    if (s == NULL || s[0] == '\0')
    {
        return 0;
    }

    for (i = 0; s[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)s[i]))
        {
            return 0;
        }
    }

    return 1;
}

static void trim_left_zeros(char *s)
{
    size_t i;
    size_t len;

    if (s == NULL)
    {
        return;
    }

    len = strlen(s);
    i = 0;
    while (i + 1U < len && s[i] == '0')
    {
        i++;
    }

    if (i > 0U)
    {
        memmove(s, s + i, len - i + 1U);
    }
}

static int bd_normalize(BigDecimal *bd)
{
    if (bd == NULL || bd->digits == NULL || !is_all_digits(bd->digits))
    {
        return 0;
    }

    trim_left_zeros(bd->digits);
    bd->precision = (int)strlen(bd->digits);

    while (bd->scale > 0 && bd->precision > 1 && bd->digits[bd->precision - 1] == '0')
    {
        bd->precision--;
        bd->scale--;
        bd->digits[bd->precision] = '\0';
    }

    if (strcmp(bd->digits, "0") == 0)
    {
        bd->sign = 1;
        bd->scale = 0;
        bd->precision = 1;
    }

    return 1;
}

static BigDecimal *bd_alloc_from_parts(char *digits, int sign, int scale)
{
    BigDecimal *bd;

    if (digits == NULL || !is_all_digits(digits) || scale < 0)
    {
        free(digits);
        return NULL;
    }

    bd = (BigDecimal *)malloc(sizeof(BigDecimal));
    if (bd == NULL)
    {
        free(digits);
        return NULL;
    }

    bd->digits = digits;
    bd->sign = (sign >= 0) ? 1 : -1;
    bd->scale = scale;
    bd->precision = (int)strlen(digits);

    if (!bd_normalize(bd))
    {
        bd_free(bd);
        return NULL;
    }

    return bd;
}

static char *append_zeros_right(const char *digits, int zeros)
{
    size_t len;
    char *out;
    int i;

    if (digits == NULL || zeros < 0)
    {
        return NULL;
    }

    len = strlen(digits);
    out = (char *)malloc(len + (size_t)zeros + 1U);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, digits, len);
    for (i = 0; i < zeros; i++)
    {
        out[len + (size_t)i] = '0';
    }
    out[len + (size_t)zeros] = '\0';

    return out;
}

static int compare_abs_same_scale(const BigDecimal *a, const BigDecimal *b, int target_scale)
{
    int len_a;
    int len_b;
    int i;

    len_a = a->precision + (target_scale - a->scale);
    len_b = b->precision + (target_scale - b->scale);

    if (len_a > len_b)
    {
        return 1;
    }
    if (len_a < len_b)
    {
        return -1;
    }

    for (i = 0; i < len_a; i++)
    {
        char da = (i < a->precision) ? a->digits[i] : '0';
        char db = (i < b->precision) ? b->digits[i] : '0';
        if (da > db)
        {
            return 1;
        }
        if (da < db)
        {
            return -1;
        }
    }

    return 0;
}

static char *add_abs_str(const char *x, const char *y)
{
    size_t lx;
    size_t ly;
    size_t lmax;
    char *res;
    size_t i;
    int carry;

    if (x == NULL || y == NULL)
    {
        return NULL;
    }

    lx = strlen(x);
    ly = strlen(y);
    lmax = (lx > ly) ? lx : ly;

    res = (char *)malloc(lmax + 2U);
    if (res == NULL)
    {
        return NULL;
    }

    res[lmax + 1U] = '\0';
    carry = 0;

    for (i = 0; i < lmax; i++)
    {
        int dx = (int)((lx > i) ? (x[lx - 1U - i] - '0') : 0);
        int dy = (int)((ly > i) ? (y[ly - 1U - i] - '0') : 0);
        int s = dx + dy + carry;
        res[lmax - i] = (char)('0' + (s % 10));
        carry = s / 10;
    }

    res[0] = (char)('0' + carry);
    if (res[0] == '0')
    {
        memmove(res, res + 1, lmax + 1U);
    }

    return res;
}

static int compare_abs_int_str(const char *x, const char *y)
{
    size_t lx;
    size_t ly;
    int cmp;

    if (x == NULL || y == NULL)
    {
        return 0;
    }

    lx = strlen(x);
    ly = strlen(y);

    if (lx > ly)
    {
        return 1;
    }
    if (lx < ly)
    {
        return -1;
    }

    cmp = strcmp(x, y);
    if (cmp > 0)
    {
        return 1;
    }
    if (cmp < 0)
    {
        return -1;
    }
    return 0;
}

static char *sub_abs_str(const char *x, const char *y)
{
    size_t lx;
    size_t ly;
    char *res;
    size_t i;
    int borrow;

    if (x == NULL || y == NULL || compare_abs_int_str(x, y) < 0)
    {
        return NULL;
    }

    lx = strlen(x);
    ly = strlen(y);
    res = (char *)malloc(lx + 1U);
    if (res == NULL)
    {
        return NULL;
    }

    res[lx] = '\0';
    borrow = 0;

    for (i = 0; i < lx; i++)
    {
        int dx = (int)(x[lx - 1U - i] - '0') - borrow;
        int dy = (int)((ly > i) ? (y[ly - 1U - i] - '0') : 0);
        int d;

        if (dx < dy)
        {
            dx += 10;
            borrow = 1;
        }
        else
        {
            borrow = 0;
        }

        d = dx - dy;
        res[lx - 1U - i] = (char)('0' + d);
    }

    trim_left_zeros(res);
    return res;
}

static char *mul_abs_str(const char *x, const char *y)
{
    size_t lx;
    size_t ly;
    size_t i;
    size_t j;
    int *tmp;
    char *res;

    if (x == NULL || y == NULL)
    {
        return NULL;
    }

    if ((x[0] == '0' && x[1] == '\0') || (y[0] == '0' && y[1] == '\0'))
    {
        return bd_strdup("0");
    }

    lx = strlen(x);
    ly = strlen(y);
    tmp = (int *)calloc(lx + ly, sizeof(int));
    if (tmp == NULL)
    {
        return NULL;
    }

    for (i = lx; i > 0; i--)
    {
        for (j = ly; j > 0; j--)
        {
            size_t p = i + j - 1U;
            int m = (x[i - 1U] - '0') * (y[j - 1U] - '0');
            int s = tmp[p] + m;
            tmp[p] = s % 10;
            tmp[p - 1U] += s / 10;
        }
    }

    res = (char *)malloc(lx + ly + 1U);
    if (res == NULL)
    {
        free(tmp);
        return NULL;
    }

    for (i = 0; i < lx + ly; i++)
    {
        res[i] = (char)('0' + tmp[i]);
    }
    res[lx + ly] = '\0';

    free(tmp);
    trim_left_zeros(res);
    return res;
}

static char *mul10_add_digit(const char *num, char digit)
{
    size_t len;
    char *out;

    if (num == NULL || digit < '0' || digit > '9')
    {
        return NULL;
    }

    if (num[0] == '0' && num[1] == '\0')
    {
        out = (char *)malloc(2U);
        if (out == NULL)
        {
            return NULL;
        }
        out[0] = digit;
        out[1] = '\0';
        trim_left_zeros(out);
        return out;
    }

    len = strlen(num);
    out = (char *)malloc(len + 2U);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, num, len);
    out[len] = digit;
    out[len + 1U] = '\0';
    trim_left_zeros(out);
    return out;
}

static char *div_abs_str(const char *dividend, const char *divisor)
{
    size_t n;
    size_t i;
    char *quot;
    char *rem;

    if (dividend == NULL || divisor == NULL)
    {
        return NULL;
    }

    if (divisor[0] == '0' && divisor[1] == '\0')
    {
        return NULL;
    }

    if (compare_abs_int_str(dividend, divisor) < 0)
    {
        return bd_strdup("0");
    }

    n = strlen(dividend);
    quot = (char *)malloc(n + 1U);
    if (quot == NULL)
    {
        return NULL;
    }

    rem = bd_strdup("0");
    if (rem == NULL)
    {
        free(quot);
        return NULL;
    }

    for (i = 0; i < n; i++)
    {
        char *next_rem;
        int qdigit;

        next_rem = mul10_add_digit(rem, dividend[i]);
        free(rem);
        rem = next_rem;
        if (rem == NULL)
        {
            free(quot);
            return NULL;
        }

        qdigit = 0;
        while (compare_abs_int_str(rem, divisor) >= 0)
        {
            char *new_rem = sub_abs_str(rem, divisor);
            free(rem);
            rem = new_rem;
            if (rem == NULL)
            {
                free(quot);
                return NULL;
            }
            qdigit++;
        }

        quot[i] = (char)('0' + qdigit);
    }

    quot[n] = '\0';
    free(rem);
    trim_left_zeros(quot);
    return quot;
}

static char *ensure_min_digits(char *digits, int min_digits)
{
    int len;
    int pad;
    char *out;

    if (digits == NULL)
    {
        return NULL;
    }

    len = (int)strlen(digits);
    if (len >= min_digits)
    {
        return digits;
    }

    pad = min_digits - len;
    out = (char *)malloc((size_t)min_digits + 1U);
    if (out == NULL)
    {
        free(digits);
        return NULL;
    }

    memset(out, '0', (size_t)pad);
    memcpy(out + pad, digits, (size_t)len + 1U);
    free(digits);
    return out;
}

BigDecimal *bd_create_from_str(const char *str)
{
    int sign;
    size_t len;
    size_t i;
    int seen_dot;
    int scale;
    size_t nd;
    char *digits;

    if (str == NULL)
    {
        return NULL;
    }

    while (*str != '\0' && isspace((unsigned char)*str))
    {
        str++;
    }

    sign = 1;
    if (*str == '+')
    {
        str++;
    }
    else if (*str == '-')
    {
        sign = -1;
        str++;
    }

    len = strlen(str);
    while (len > 0U && isspace((unsigned char)str[len - 1U]))
    {
        len--;
    }

    if (len == 0U)
    {
        return NULL;
    }

    digits = (char *)malloc(len + 1U);
    if (digits == NULL)
    {
        return NULL;
    }

    seen_dot = 0;
    scale = 0;
    nd = 0U;

    for (i = 0; i < len; i++)
    {
        char c = str[i];

        if (c == '.')
        {
            if (seen_dot)
            {
                free(digits);
                return NULL;
            }
            seen_dot = 1;
            continue;
        }

        if (!isdigit((unsigned char)c))
        {
            free(digits);
            return NULL;
        }

        digits[nd++] = c;
        if (seen_dot)
        {
            scale++;
        }
    }

    if (nd == 0U)
    {
        free(digits);
        return NULL;
    }

    digits[nd] = '\0';
    return bd_alloc_from_parts(digits, sign, scale);
}

BigDecimal *bd_create_from_int(long long value)
{
    char buffer[64];
    int sign;
    unsigned long long magnitude;

    sign = (value < 0) ? -1 : 1;
    if (value == LLONG_MIN)
    {
        magnitude = ((unsigned long long)LLONG_MAX) + 1ULL;
    }
    else
    {
        magnitude = (unsigned long long)((value < 0) ? -value : value);
    }

    (void)snprintf(buffer, sizeof(buffer), "%llu", magnitude);
    return bd_alloc_from_parts(bd_strdup(buffer), sign, 0);
}

BigDecimal *bd_copy(const BigDecimal *src)
{
    if (src == NULL)
    {
        return NULL;
    }

    return bd_alloc_from_parts(bd_strdup(src->digits), src->sign, src->scale);
}

void bd_free(BigDecimal *bd)
{
    if (bd == NULL)
    {
        return;
    }

    free(bd->digits);
    free(bd);
}

bool bd_is_zero(const BigDecimal *bd)
{
    return bd != NULL && bd->digits != NULL && bd->digits[0] == '0' && bd->digits[1] == '\0';
}

int bd_compare(const BigDecimal *a, const BigDecimal *b)
{
    int target_scale;
    int abs_cmp;

    if (a == NULL || b == NULL)
    {
        return 0;
    }

    if (a->sign != b->sign)
    {
        return (a->sign > b->sign) ? 1 : -1;
    }

    target_scale = (a->scale > b->scale) ? a->scale : b->scale;
    abs_cmp = compare_abs_same_scale(a, b, target_scale);
    return (a->sign > 0) ? abs_cmp : -abs_cmp;
}

BigDecimal *bd_add(const BigDecimal *a, const BigDecimal *b)
{
    int target_scale;
    char *sa;
    char *sb;
    char *digits;
    int sign;
    int cmp;

    if (a == NULL || b == NULL)
    {
        return NULL;
    }

    target_scale = (a->scale > b->scale) ? a->scale : b->scale;
    sa = append_zeros_right(a->digits, target_scale - a->scale);
    sb = append_zeros_right(b->digits, target_scale - b->scale);
    if (sa == NULL || sb == NULL)
    {
        free(sa);
        free(sb);
        return NULL;
    }

    if (a->sign == b->sign)
    {
        digits = add_abs_str(sa, sb);
        sign = a->sign;
    }
    else
    {
        cmp = compare_abs_int_str(sa, sb);
        if (cmp == 0)
        {
            free(sa);
            free(sb);
            return bd_create_from_int(0);
        }
        if (cmp > 0)
        {
            digits = sub_abs_str(sa, sb);
            sign = a->sign;
        }
        else
        {
            digits = sub_abs_str(sb, sa);
            sign = b->sign;
        }
    }

    free(sa);
    free(sb);

    if (digits == NULL)
    {
        return NULL;
    }

    return bd_alloc_from_parts(digits, sign, target_scale);
}

BigDecimal *bd_subtract(const BigDecimal *a, const BigDecimal *b)
{
    BigDecimal temp;

    if (a == NULL || b == NULL)
    {
        return NULL;
    }

    temp = *b;
    temp.sign = -temp.sign;
    return bd_add(a, &temp);
}

BigDecimal *bd_multiply(const BigDecimal *a, const BigDecimal *b)
{
    char *digits;

    if (a == NULL || b == NULL)
    {
        return NULL;
    }

    digits = mul_abs_str(a->digits, b->digits);
    if (digits == NULL)
    {
        return NULL;
    }

    return bd_alloc_from_parts(digits, a->sign * b->sign, a->scale + b->scale);
}

BigDecimal *bd_divide(const BigDecimal *a, const BigDecimal *b, int limit_precision)
{
    char *num;
    char *den;
    char *scaled_num;
    char *digits;
    int exp;
    int min_digits;

    if (a == NULL || b == NULL || limit_precision < 0)
    {
        return NULL;
    }

    if (bd_is_zero(b))
    {
        return NULL;
    }

    num = bd_strdup(a->digits);
    den = bd_strdup(b->digits);
    if (num == NULL || den == NULL)
    {
        free(num);
        free(den);
        return NULL;
    }

    exp = b->scale - a->scale;
    if (exp >= 0)
    {
        char *tmp = append_zeros_right(num, exp);
        free(num);
        num = tmp;
    }
    else
    {
        char *tmp = append_zeros_right(den, -exp);
        free(den);
        den = tmp;
    }

    if (num == NULL || den == NULL)
    {
        free(num);
        free(den);
        return NULL;
    }

    scaled_num = append_zeros_right(num, limit_precision);
    free(num);
    if (scaled_num == NULL)
    {
        free(den);
        return NULL;
    }

    digits = div_abs_str(scaled_num, den);
    free(scaled_num);
    free(den);
    if (digits == NULL)
    {
        return NULL;
    }

    min_digits = (limit_precision == 0) ? 1 : (limit_precision + 1);
    digits = ensure_min_digits(digits, min_digits);
    if (digits == NULL)
    {
        return NULL;
    }

    return bd_alloc_from_parts(digits, a->sign * b->sign, limit_precision);
}

char *bd_to_string(const BigDecimal *bd)
{
    size_t digits_len;
    size_t sign_len;
    size_t int_len;
    size_t out_len;
    char *out;

    if (bd == NULL || bd->digits == NULL)
    {
        return NULL;
    }

    digits_len = (size_t)bd->precision;
    sign_len = (bd->sign < 0 && !bd_is_zero(bd)) ? 1U : 0U;

    if (bd->scale == 0)
    {
        out_len = sign_len + digits_len;
        out = (char *)malloc(out_len + 1U);
        if (out == NULL)
        {
            return NULL;
        }

        if (sign_len == 1U)
        {
            out[0] = '-';
        }
        memcpy(out + sign_len, bd->digits, digits_len + 1U);
        return out;
    }

    if (digits_len > (size_t)bd->scale)
    {
        int_len = digits_len - (size_t)bd->scale;
        out_len = sign_len + int_len + 1U + (size_t)bd->scale;
        out = (char *)malloc(out_len + 1U);
        if (out == NULL)
        {
            return NULL;
        }

        if (sign_len == 1U)
        {
            out[0] = '-';
        }
        memcpy(out + sign_len, bd->digits, int_len);
        out[sign_len + int_len] = '.';
        memcpy(out + sign_len + int_len + 1U, bd->digits + int_len, (size_t)bd->scale);
        out[out_len] = '\0';
        return out;
    }

    out_len = sign_len + 2U + ((size_t)bd->scale - digits_len) + digits_len;
    out = (char *)malloc(out_len + 1U);
    if (out == NULL)
    {
        return NULL;
    }

    if (sign_len == 1U)
    {
        out[0] = '-';
    }
    out[sign_len] = '0';
    out[sign_len + 1U] = '.';
    memset(out + sign_len + 2U, '0', (size_t)bd->scale - digits_len);
    memcpy(out + sign_len + 2U + ((size_t)bd->scale - digits_len), bd->digits, digits_len);
    out[out_len] = '\0';
    return out;
}

void bd_print(const BigDecimal *bd)
{
    char *s;

    s = bd_to_string(bd);
    if (s == NULL)
    {
        return;
    }

    printf("%s", s);
    free(s);
}