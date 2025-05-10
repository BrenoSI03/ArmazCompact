/**
 * Carolina de Assis Souza 2320860 3WC 
 * Breno de Andrade Soares 2320363 3WC 
 * 
 * gravacomp.c
 * 
 * Este módulo implementa a compactação e exibição de arrays de structs em arquivos binários.
 * A função `gravacomp` escreve os dados de forma compacta com cabeçalhos específicos,
 * enquanto `mostracomp` exibe os dados em formato legível, de acordo com os tipos definidos.
 *
 */
#include "gravacomp.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ---------- UTILITÁRIOS ---------- */

/** 
 * Arredonda um valor para múltiplo de 4 (alinhamento).
 *
 * @param x O tamanho a ser alinhado.
 * @return O tamanho alinhado.
 */
static int align4(int x) { 
    return (x + 3u) & ~3u; 
}

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
 * Grava um valor inteiro (com ou sem sinal) no arquivo em formato compactado.
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
    if (ultimo)   h |= 0x80;         /* Define bit de último campo (bit-7) - Bit Mais Significativo */
    if (assinado) h |= 0x20;         /* Tipo: signed (bits 6-5: 01) */
    h |= (unsigned char)nbytes;      /* Número de bytes usados (bits 4-0 - Tamanho) */
    fwrite(&h, 1, 1, f);

    // Grava os bytes do valor, em ordem big-endian
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
static void grava_str(FILE *f, const char *s, int tam_util, int ultimo)
{
    int tam = tam_util;                    // Número de bytes usados (bits 5-0 - Tamanho)

    unsigned char h = 0x40 | (tam & 0x3F); // Tipo: string (bit 6 = 1)
    if (ultimo) h |= 0x80;                 // Define bit de último campo (bit-7) - Bit Mais Significativo
    fwrite(&h, 1, 1, f);
    fwrite(s, 1, (int)tam, f);
}

/* ---------- FUNÇÃO PRINCIPAL: GRAVAR ----------- */

/** 
 * Função principal para gravar os dados em um arquivo no formato compactado.
 *
 * @param nstructs O número de structs a serem gravadas (máximo 255).
 * @param valores O ponteiro para para o início do array de structs.
 * @param desc O descritor com o layout dos campos das structs
 * @param f O ponteiro para o arquivo binário aberto para escrita.
 * @return 0 em caso de sucesso, -1 em caso de erro de E/S ou descritor inválido.
 */
int gravacomp(int nstructs, void *valores, char *desc, FILE *f)
{
    if (nstructs < 0 || nstructs > 255) return -1;
    if (fwrite(&(unsigned char){nstructs}, 1, 1, f) != 1) return -1;

    // Calcula o tamanho da struct com base no descritor
    int tam_struct = 0, i = 0;
    while (desc[i]) {
        if (desc[i] == 'i' || desc[i] == 'u') {
            tam_struct = align4(tam_struct) + 4;
            i++;
        } else if (desc[i] == 's') {
            int tam = 0; i++;
            while (isdigit((unsigned char)desc[i]))
                tam = tam*10 + (desc[i++] - '0');
            if (tam < 1 || tam > 64) return -1;
            tam_struct += (int)tam;
        } else return -1;
    }
    tam_struct = align4(tam_struct);

    unsigned char *base = valores;

    for (int s = 0; s < nstructs; ++s, base += tam_struct) {
        int offset = 0; 
        int i   = 0;

        while (desc[i]) {
            int ultimo;
            if (desc[i] == 'i' || desc[i] == 'u') {
                ultimo = (desc[i + 1] == '\0');
            } else {
                int j = i + 1;
                while (isdigit((unsigned char)desc[j])) ++j;
                ultimo = (desc[j] == '\0');
            }

            if (desc[i] == 'i' || desc[i] == 'u') {
                offset = align4(offset);
                int val;
                memcpy(&val, base + offset, 4);
                grava_int(f, val, desc[i] == 'i', ultimo);
                offset += 4;
                ++i;

            } else {
                int tam = 0;
                int j = i + 1;
                while (isdigit((unsigned char)desc[j]))
                    tam = tam * 10 + (desc[j++] - '0');
                int real_len = (int)strnlen((char *)(base + offset), tam);
                grava_str(f, (char *)(base + offset), real_len, ultimo);
                offset += tam;
                i = j;
            }
        }
    }
    return ferror(f) ? -1 : 0;
}

/* ---------- FUNÇÃO PRINCIPAL: MOSTRAR ---------- */

/** 
 * Função principal para ler e exibir dados compactados de um arquivo.
 *
 * @param f O ponteiro para o arquivo aberto para leitura em modo binário.
 */
void mostracomp(FILE *f)
{
    int n = fgetc(f);
    if (n == EOF) return;

    printf("Estruturas: %d\n\n", n);

    for (int s = 0; s < n; s++) {
        int done = 0;

        // Lê os campos até encontrar o último da estrutura
        while (!done) {
            int h = fgetc(f); if (h == EOF) return;
            done = h & 0x80;

            if (h & 0x40) { // Campo do tipo string
                int tam = h & 0x3F;
                char buf[64]; fread(buf, 1, tam, f);
                buf[tam] = '\0';
                printf("(str) %s\n", buf);

            } else { // Campo do tipo inteiro
                int tipo   = (h >> 5) & 3;  // 0 = unsigned, 1 = signed
                int nb     = h & 0x1F;
                unsigned val = 0;
                for (int i = 0; i < nb; i++)
                    val = (val << 8) | (unsigned)fgetc(f);

                if (tipo == 0) { 
                    printf("(uns) %u (%08x)\n", val, val);
                } else { 
                    int sval = (nb == 4 || !(val & (1u << (nb*8-1))))
                               ? (int)val
                               : (int)(val | (~0u << (nb*8)));
                    printf("(int) %d (%08x)\n", sval, (unsigned)sval);
                }
            }
        }     
        printf("\n");
    }
}
