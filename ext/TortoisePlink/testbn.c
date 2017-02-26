/*
 * testbn.c: standalone test program for the bignum code.
 */

/*
 * Accepts input on standard input, in the form generated by
 * testdata/bignum.py.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "ssh.h"
#include "sshbn.h"

void modalfatalbox(const char *p, ...)
{
    va_list ap;
    fprintf(stderr, "FATAL ERROR: ");
    va_start(ap, p);
    vfprintf(stderr, p, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

int random_byte(void)
{
    modalfatalbox("random_byte called in testbn");
    return 0;
}

#define fromxdigit(c) ( (c)>'9' ? ((c)&0xDF) - 'A' + 10 : (c) - '0' )

int main(int argc, char **argv)
{
    char *buf;
    int line = 0;
    int passes = 0, fails = 0;

    printf("BIGNUM_INT_BITS = %d\n", (int)BIGNUM_INT_BITS);

    while ((buf = fgetline(stdin)) != NULL) {
        int maxlen = strlen(buf);
        unsigned char *data = snewn(maxlen, unsigned char);
        unsigned char *ptrs[5], *q;
        int ptrnum;
        char *bufp = buf;

        line++;

        q = data;
        ptrnum = 0;

        while (*bufp && !isspace((unsigned char)*bufp))
            bufp++;
        if (*bufp)
            *bufp++ = '\0';

        while (*bufp) {
            char *start, *end;
            int i;

            while (*bufp && !isxdigit((unsigned char)*bufp))
                bufp++;
            start = bufp;

            if (!*bufp)
                break;

            while (*bufp && isxdigit((unsigned char)*bufp))
                bufp++;
            end = bufp;

            if (ptrnum >= lenof(ptrs))
                break;
            ptrs[ptrnum++] = q;
            
            for (i = -((end - start) & 1); i < end-start; i += 2) {
                unsigned char val = (i < 0 ? 0 : fromxdigit(start[i]));
                val = val * 16 + fromxdigit(start[i+1]);
                *q++ = val;
            }
        }

        if (!strcmp(buf, "mul")) {
            Bignum a, b, c, p;

            if (ptrnum != 3) {
                printf("%d: mul with %d parameters, expected 3\n", line, ptrnum);
                exit(1);
            }
            a = bignum_from_bytes(ptrs[0], ptrs[1]-ptrs[0]);
            b = bignum_from_bytes(ptrs[1], ptrs[2]-ptrs[1]);
            c = bignum_from_bytes(ptrs[2], ptrs[3]-ptrs[2]);
            p = bigmul(a, b);

            if (bignum_cmp(c, p) == 0) {
                passes++;
            } else {
                char *as = bignum_decimal(a);
                char *bs = bignum_decimal(b);
                char *cs = bignum_decimal(c);
                char *ps = bignum_decimal(p);
                
                printf("%d: fail: %s * %s gave %s expected %s\n",
                       line, as, bs, ps, cs);
                fails++;

                sfree(as);
                sfree(bs);
                sfree(cs);
                sfree(ps);
            }
            freebn(a);
            freebn(b);
            freebn(c);
            freebn(p);
        } else if (!strcmp(buf, "modmul")) {
            Bignum a, b, m, c, p;

            if (ptrnum != 4) {
                printf("%d: modmul with %d parameters, expected 4\n",
                       line, ptrnum);
                exit(1);
            }
            a = bignum_from_bytes(ptrs[0], ptrs[1]-ptrs[0]);
            b = bignum_from_bytes(ptrs[1], ptrs[2]-ptrs[1]);
            m = bignum_from_bytes(ptrs[2], ptrs[3]-ptrs[2]);
            c = bignum_from_bytes(ptrs[3], ptrs[4]-ptrs[3]);
            p = modmul(a, b, m);

            if (bignum_cmp(c, p) == 0) {
                passes++;
            } else {
                char *as = bignum_decimal(a);
                char *bs = bignum_decimal(b);
                char *ms = bignum_decimal(m);
                char *cs = bignum_decimal(c);
                char *ps = bignum_decimal(p);
                
                printf("%d: fail: %s * %s mod %s gave %s expected %s\n",
                       line, as, bs, ms, ps, cs);
                fails++;

                sfree(as);
                sfree(bs);
                sfree(ms);
                sfree(cs);
                sfree(ps);
            }
            freebn(a);
            freebn(b);
            freebn(m);
            freebn(c);
            freebn(p);
        } else if (!strcmp(buf, "pow")) {
            Bignum base, expt, modulus, expected, answer;

            if (ptrnum != 4) {
                printf("%d: pow with %d parameters, expected 4\n", line, ptrnum);
                exit(1);
            }

            base = bignum_from_bytes(ptrs[0], ptrs[1]-ptrs[0]);
            expt = bignum_from_bytes(ptrs[1], ptrs[2]-ptrs[1]);
            modulus = bignum_from_bytes(ptrs[2], ptrs[3]-ptrs[2]);
            expected = bignum_from_bytes(ptrs[3], ptrs[4]-ptrs[3]);
            answer = modpow(base, expt, modulus);

            if (bignum_cmp(expected, answer) == 0) {
                passes++;
            } else {
                char *as = bignum_decimal(base);
                char *bs = bignum_decimal(expt);
                char *cs = bignum_decimal(modulus);
                char *ds = bignum_decimal(answer);
                char *ps = bignum_decimal(expected);
                
                printf("%d: fail: %s ^ %s mod %s gave %s expected %s\n",
                       line, as, bs, cs, ds, ps);
                fails++;

                sfree(as);
                sfree(bs);
                sfree(cs);
                sfree(ds);
                sfree(ps);
            }
            freebn(base);
            freebn(expt);
            freebn(modulus);
            freebn(expected);
            freebn(answer);
        } else if (!strcmp(buf, "divmod")) {
            Bignum n, d, expect_q, expect_r, answer_q, answer_r;
            int fail;

            if (ptrnum != 4) {
                printf("%d: divmod with %d parameters, expected 4\n", line, ptrnum);
                exit(1);
            }

            n = bignum_from_bytes(ptrs[0], ptrs[1]-ptrs[0]);
            d = bignum_from_bytes(ptrs[1], ptrs[2]-ptrs[1]);
            expect_q = bignum_from_bytes(ptrs[2], ptrs[3]-ptrs[2]);
            expect_r = bignum_from_bytes(ptrs[3], ptrs[4]-ptrs[3]);
            answer_q = bigdiv(n, d);
            answer_r = bigmod(n, d);

            fail = FALSE;
            if (bignum_cmp(expect_q, answer_q) != 0) {
                char *as = bignum_decimal(n);
                char *bs = bignum_decimal(d);
                char *cs = bignum_decimal(answer_q);
                char *ds = bignum_decimal(expect_q);

                printf("%d: fail: %s / %s gave %s expected %s\n",
                       line, as, bs, cs, ds);
                fail = TRUE;

                sfree(as);
                sfree(bs);
                sfree(cs);
                sfree(ds);
            }
            if (bignum_cmp(expect_r, answer_r) != 0) {
                char *as = bignum_decimal(n);
                char *bs = bignum_decimal(d);
                char *cs = bignum_decimal(answer_r);
                char *ds = bignum_decimal(expect_r);

                printf("%d: fail: %s mod %s gave %s expected %s\n",
                       line, as, bs, cs, ds);
                fail = TRUE;

                sfree(as);
                sfree(bs);
                sfree(cs);
                sfree(ds);
            }

            freebn(n);
            freebn(d);
            freebn(expect_q);
            freebn(expect_r);
            freebn(answer_q);
            freebn(answer_r);

            if (fail)
                fails++;
            else
                passes++;
        } else {
            printf("%d: unrecognised test keyword: '%s'\n", line, buf);
            exit(1);
        }

        sfree(buf);
        sfree(data);
    }

    printf("passed %d failed %d total %d\n", passes, fails, passes+fails);
    return fails != 0;
}
