/** 
 * gravacomp.c – versão simplificada (ainda 100% funcional)
 * 
 * Este arquivo contém funções para gravar e mostrar dados estruturados em um formato compacto.
 * A principal função de gravação grava inteiros, unsigned inteiros e strings em um arquivo, enquanto a função de 
 * exibição lê esse arquivo e imprime o conteúdo no formato adequado.
 *
 * @author Nome do autor
 * @version 1.0
 */
#include "gravacomp.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---------- utilidades mínimas ---------- */

/** 
 * Alinha um tamanho para ser múltiplo de 4 bytes.
 *
 * @param x O tamanho a ser alinhado.
 * @return O tamanho alinhado.
 */
static size_t align4(size_t x) { return (x + 3u) & ~3u; }

/** 
 * Determina o número de bytes necessários para armazenar um número sem sinal.
 *
 * @param v O valor a ser analisado.
 * @return O número de bytes necessários.
 */
static int bytes_unsigned(unsigned int v)
{
    return (v > 0xFFFFFFu) ? 4 :
           (v > 0xFFFFu   ) ? 3 :
           (v > 0xFFu     ) ? 2 : 1;
}

/** 
 * Determina o número de bytes necessários para armazenar um número com sinal (complemento de dois).
 *
 * @param v O valor a ser analisado.
 * @return O número de bytes necessários.
 */
static int bytes_signed(int v)
{
    int n = 4;
    while (n > 1) {
        unsigned char msb  = (v >> ((n-1)*8)) & 0xFF;
        unsigned char nmsb = (v >> ((n-2)*8)) & 0xFF;
        if ((msb == 0x00 && (nmsb & 0x80) == 0) ||
            (msb == 0xFF && (nmsb & 0x80) != 0))
            n--;
        else break;
    }
    return n;
}

/** 
 * Grava um valor inteiro ou unsigned inteiro no arquivo em formato compactado.
 *
 * @param f O ponteiro para o arquivo onde os dados serão gravados.
 * @param val O valor a ser gravado.
 * @param assinado Se verdadeiro, o valor é tratado como inteiro com sinal.
 * @param ultimo Se verdadeiro, o campo é o último no bloco de dados.
 */
static void grava_int(FILE *f, int val, int assinado, int ultimo)
{
    int nbytes = assinado ? bytes_signed(val)
                          : bytes_unsigned((unsigned)val);

    unsigned char h = 0;
    if (ultimo)   h |= 0x80;         /* bit-7       */
    if (assinado) h |= 0x20;         /* bits 6-5: 01 = signed */
    h |= (unsigned char)nbytes;      /* bits 4-0    */
    fwrite(&h, 1, 1, f);

    for (int i = nbytes-1; i >= 0; --i) {
        unsigned char b = assinado ? ((unsigned int)val >> (i*8)) : ((unsigned int)val >> (i*8));
        fwrite(&b, 1, 1, f);
    }
}

/** 
 * Grava uma string no arquivo em formato compactado.
 *
 * @param f O ponteiro para o arquivo onde os dados serão gravados.
 * @param s A string a ser gravada.
 * @param ultimo Se verdadeiro, a string é a última no bloco de dados.
 */
static void grava_str(FILE *f, const char *s, int ultimo)
{
    int tam = (int)strlen(s);
    if (tam > 63) tam = 63;

    unsigned char h = 0x40 | (tam & 0x3F); /* bit-6 = 1, bits 5-0 = tam */
    if (ultimo) h |= 0x80;
    fwrite(&h, 1, 1, f);
    fwrite(s, 1, (size_t)tam, f);
}

/* ---------- função principal: GRAVAR ----------- */

/** 
 * Função principal para gravar os dados em um arquivo no formato compactado.
 *
 * @param nstructs O número de structs a serem gravadas.
 * @param valores O ponteiro para os valores a serem gravados.
 * @param desc A descrição do formato das structs.
 * @param f O ponteiro para o arquivo onde os dados serão gravados.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int gravacomp(int nstructs, void *valores, char *desc, FILE *f)
{
    if (nstructs < 0 || nstructs > 255) return -1;
    if (fwrite(&(unsigned char){nstructs}, 1, 1, f) != 1) return -1;

    /* 1ª passada pelo descritor apenas para saber tamanho da struct */
    size_t tam_struct = 0, i = 0;
    while (desc[i]) {
        if (desc[i] == 'i' || desc[i] == 'u') {
            tam_struct = align4(tam_struct) + 4;
            i++;
        } else if (desc[i] == 's') {
            int tam = 0; i++;
            while (isdigit((unsigned char)desc[i]))
                tam = tam*10 + (desc[i++] - '0');
            if (tam < 1 || tam > 64) return -1;
            tam_struct += (size_t)tam;
        } else return -1;
    }
    tam_struct = align4(tam_struct);

    /* percorre cada struct */
    unsigned char *base = valores;
    for (int s = 0; s < nstructs; s++, base += tam_struct) {

        size_t off = 0;  i = 0;      /* reinicia ponteiros do descritor */

        /* percorre cada campo */
        while (desc[i]) {
            int ultimo = (desc[i+1] == '\0' ||    /* próximo char   */
                          (desc[i]=='s' && !isdigit((unsigned char)desc[i+2])));

            if (desc[i] == 'i' || desc[i] == 'u') {
                off = align4(off);
                int val; memcpy(&val, base + off, 4);
                grava_int(f, val, desc[i]=='i', ultimo);
                off += 4; i++;

            } else {                    /* string */
                int tam = 0; i++; size_t j = i;
                while (isdigit((unsigned char)desc[j]))
                    tam = tam*10 + (desc[j++] - '0');
                grava_str(f, (char*)(base + off), ultimo);
                off += (size_t)tam;
                i = j;
            }
        }
    }
    return ferror(f) ? -1 : 0;
}

/* ---------- função principal: MOSTRAR ---------- */

/** 
 * Função principal para ler e exibir dados compactados de um arquivo.
 *
 * @param f O ponteiro para o arquivo de onde os dados serão lidos.
 */
void mostracomp(FILE *f)
{
    int n = fgetc(f);
    if (n == EOF) return;
    printf("Estruturas: %d\n\n", n);

    for (int s = 0; s < n; s++) {
        int done = 0;
        while (!done) {
            int h = fgetc(f); if (h == EOF) return;
            done = h & 0x80;

            if (h & 0x40) {                 /* string */
                int tam = h & 0x3F;
                char buf[64]; fread(buf, 1, tam, f);
                buf[tam] = '\0';
                printf("(str) %s\n", buf);

            } else {                        /* inteiro */
                int tipo   = (h >> 5) & 3;  /* 0 = uns, 1 = int */
                int nb     = h & 0x1F;
                unsigned val = 0;
                for (int i = 0; i < nb; i++)
                    val = (val << 8) | (unsigned)fgetc(f);

                if (tipo == 0) {            /* unsigned */
                    printf("(uns) %u (%08x)\n", val, val);
                } else {                    /* signed */
                    int sval = (nb == 4 || !(val & (1u << (nb*8-1))))
                               ? (int)val
                               : (int)(val | (~0u << (nb*8)));
                    printf("(int) %d (%08x)\n", sval, (unsigned)sval);
                }
            }
        }
        if (s != n-1) {
            putchar('\n');
            fflush(stdout);  // Força a impressão imediata
        }        
    }
}
