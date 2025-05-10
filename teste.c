#include "gravacomp.h"
#include <stdio.h>

struct s {
    int i;
    char s1[4];
    unsigned u;
    char s2[5];
};

struct A {
    int a;
    int b;
    int c;
};

struct B {
    char c;
    int i;
    unsigned u;
};

struct C {
    int i;
    char s1[4];
    unsigned u;
    char s2[5];
};

void testa_struct(int n, void *dados, char *descritor,  char *arquivo) {
    FILE *fout = fopen(arquivo, "wb");
    if (!fout) { perror("fopen"); return; }
    gravacomp(n, dados, descritor, fout);
    fclose(fout);

    FILE *fin = fopen(arquivo, "rb");
    if (!fin) { perror("fopen"); return; }
    printf("\n==== Lendo %s ====\n", arquivo);
    mostracomp(fin);
    fclose(fin);
}

int main(void)
{
    struct s vs[3] = {
        { -128, "abc", 256, "defg" },
        { 999, "xyz", 1024, "ghijk" },
        { -2555, "gjh", 23, "ghjts" },
    };
    testa_struct(3, vs, "is04us05", "out_s.bin");

    struct A va[2] = {
        { 1, 2, 3 },
        { -4, -5, -6 }
    };
    testa_struct(2, va, "iii", "out_a.bin");

    struct B vb[2] = {
        { 'A', 42, 1000 },
        { 'B', -123, 65535 }
    };
    testa_struct(2, vb, "s01iu", "out_b.bin");

    struct C vc[3] = {
        { 77, "abc", 100, "xy" },
        { -88, "qqq", 222, "zzz" },
        { 255, "xyz", 500, "asdad"}
    };
    testa_struct(3, vc, "is04us05", "out_c.bin");

    return 0;
}
