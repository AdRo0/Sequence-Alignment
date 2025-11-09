#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

int **table;

uint8_t *v, *w;
size_t v_len, w_len;

int GAP_PENALTY = -2;
int match_score(uint8_t a, uint8_t b)
{
    return (a == b) ? 1 : -1;
}

int max(int a, int b, int c)
{
    int max = a;
    if (b > max)
        max = b;
    if (c > max)
        max = c;
    return max > 0 ? max : 0;
}

void smith_waterman()
{
    if (v_len * w_len > 2000000)
    {
        printf("Alignment exceeds memory limit! Try with smaller files.");
        return;
    }

    table = malloc((v_len + 1) * sizeof(int *));
    for (size_t i = 0; i < v_len + 1; i++)
        table[i] = malloc((w_len + 1) * sizeof(int));

    for (int i = 0; i < v_len + 1; i++)
    {
        for (int j = 0; j < w_len + 1; j++)
        {
            if (i == 0 || j == 0)
            {
                table[i][j] = 0;
                continue;
            }

            table[i][j] = max(table[i - 1][j - 1] + match_score(v[i - 1], w[j - 1]),
                              table[i][j - 1] + GAP_PENALTY, table[i - 1][j] + GAP_PENALTY);
        }
    }
}

void print_byte(uint8_t b)
{
    if (isprint(b))
    {
        printf("%c", b);
    }
    else
    {
        printf("NA");
    }
}

void print_text_solution()
{
    int high_i = 0, high_j = 0;
    int high = 0;

    for (size_t i = 1; i <= v_len; i++)
    {
        for (size_t j = 1; j <= w_len; j++)
        {
            int val = table[i][j];
            /* choose strictly greater score, or on tie prefer deeper cell (larger i, then larger j) */
            if (val > high || (val == high && ((int)i > high_i || ((int)i == high_i && (int)j > high_j))))
            {
                high = val;
                high_i = (int)i;
                high_j = (int)j;
            }
        }
    }

    if (high == 0)
    {
        printf("No positive-scoring alignment found.\n");
        return;
    }

    /* allocate buffer sized by the maximum possible alignment length (min of indices)
       plus one for a trailing NUL if needed */
    size_t maxlen = (high_i < high_j) ? (size_t)high_i : (size_t)high_j;
    uint8_t *a1 = malloc(maxlen + 1);
    uint8_t *a2 = malloc(maxlen + 1);
    if (!a1 || !a2)
    {
        perror("malloc");
        free(a1);
        free(a2);
        return;
    }

    size_t idx = 0;
    int ii = high_i;
    int jj = high_j;
    while (ii > 0 && jj > 0 && table[ii][jj] > 0 && idx < maxlen)
    {
        a1[idx] = v[ii - 1];
        a2[idx] = w[jj - 1];
        idx++;
        ii--;
        jj--;
    }

    /* print a1 then a2 in forward order (we filled them from end->start) */
    printf("Alignment (score %d):\n", high);
    for (size_t k = 0; k < idx; ++k)
        print_byte(a1[idx - 1 - k]);
    printf("\n");
    for (size_t k = 0; k < idx; ++k)
        print_byte(a2[idx - 1 - k]);
    printf("\n");

    free(a1);
    free(a2);
}

void print_table()
{
    if (!table)
        return;
    for (size_t i = 0; i < v_len + 1; ++i)
    {
        for (size_t j = 0; j < w_len + 1; ++j)
        {
            printf("%4d ", table[i][j]);
        }
        printf("\n");
    }
}

void print_sequences()
{
    if (v)
    {
        printf("v (len %zu):\n", v_len);
        fwrite((const void *)v, 1, v_len, stdout);
        printf("\n\n");
    }
    if (w)
    {
        printf("w (len %zu):\n", w_len);
        fwrite((const void *)w, 1, w_len, stdout);
        printf("\n\n");
    }
}

uint8_t *read_file(const char *filename, size_t *out_size)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        perror("fopen");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    uint8_t *buffer = malloc(size);
    if (!buffer)
    {
        perror("malloc");
        fclose(f);
        return NULL;
    }

    if (fread(buffer, 1, size, f) != size)
    {
        perror("fread");
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *out_size = size;
    return buffer;
}

int main(int argc, char *argv[])
{
    int n = 0;
    int hex_mode = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc)
        {
            n = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0)
        {
            hex_mode = 1;
        }
        else
        {
            if (i == 1)
                v = argv[i];
            else if (i == 2)
                w = argv[i];
            else
            {
                printf("ERROR! Argument \"%s\" unknown.", argv[i]);
                return 1;
            }
        }
    }

    v = read_file((const char *)v, &v_len);
    w = read_file((const char *)w, &w_len);

    smith_waterman();
    // print_sequences();
    // print_table();
    print_text_solution();

    return 0;
}