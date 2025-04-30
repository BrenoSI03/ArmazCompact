#include "gravacomp.h"

struct s {
    int i;
    char s1[4];
    unsigned u;
    char s2[5];
};

int main(void)
{
    struct s v[2] = {
        { -128, "abc", 256, "defg" },
        { 999, "xyz", 1024, "ghijk" }
    };

    FILE *fout = fopen("out4.bin", "wb");
    if (!fout) { perror("fopen"); return 1; }
    gravacomp(2, v, "is04us05", fout);
    fclose(fout);

    FILE *fin = fopen("out4.bin", "rb");
    if (!fin) { perror("fopen"); return 1; }
    mostracomp(fin);
    fclose(fin);

    return 0;
}
