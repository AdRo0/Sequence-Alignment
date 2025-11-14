#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

int **table;

uint8_t *v, *w;
size_t v_len, w_len;

int GAP_PENALTY = -2;
int MATCH = 1;
int MISMATCH = -1;

int match_score(uint8_t a, uint8_t b)
{
    return (a == b) ? MATCH : MISMATCH;
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
        printf("%c", b);
    else
        printf("--");
}

void print_byte_hex(uint8_t b)
{
    if (b == '_')
        printf("_ ");
    else
        printf("%02X ", b);
}

void print_solution(int n, int hex_mode)
{
    int high_i = 0, high_j = 0;
    int prev_i = 0, prev_j = 0;
    int high = 0;
    for (int k = 0; k < n; k++)
    {
        for (size_t i = 1; i <= v_len; i++)
        {
            for (size_t j = 1; j <= w_len; j++)
            {
                int val = table[i][j];
                if (val >= high && (prev_i != (int)i || prev_j != (int)j))
                {
                    high = val;
                    high_i = (int)i;
                    high_j = (int)j;
                }
            }
        }

        if (high == 0)
        {
            printf("No relevant alignment found.\n");
            return;
        }

        size_t maxlen = high_i + high_j;
        uint8_t *a1 = malloc(maxlen + 1);
        uint8_t *a2 = malloc(maxlen + 1);
        if (!a1 || !a2)
        {
            perror("malloc");
            free(a1);
            free(a2);
            return;
        }

        size_t island_size = 0;
        int curr_i = high_i;
        int curr_j = high_j;
        int scoring = 0;
        while (curr_i > 0 && curr_j > 0 && table[curr_i][curr_j] > 0 && island_size < maxlen)
        {
            int curr_score = table[curr_i][curr_j];
            table[curr_i][curr_j] = 0;
            int align_score = match_score(v[curr_i - 1], w[curr_j - 1]);
            if (curr_score == table[curr_i - 1][curr_j] + GAP_PENALTY) // Arrow Up: gap
            {
                a1[island_size] = v[--curr_i];
                a2[island_size] = '_';
                scoring += GAP_PENALTY;
            }
            else if (curr_score == table[curr_i][curr_j - 1] + GAP_PENALTY) // Arrow Left: gap
            {
                a1[island_size] = '_';
                a2[island_size] = w[--curr_j];
                scoring += GAP_PENALTY;
            }
            else // Diagonal arrow: align
            {
                a1[island_size] = v[--curr_i];
                a2[island_size] = w[--curr_j];
                scoring += align_score;
            }
            island_size++;
        }
        if (scoring != high)
        {
            k--;
            high = 0; // Reset high to search for a new maximum
            free(a1);
            free(a2);
            continue;
        }

        printf("Alignment (score %d)\n", high);
        int v_start = curr_i + 1;
        int v_end = high_i;
        int w_start = curr_j + 1;
        int w_end = high_j;
        printf("v island [%d to %d]: ", v_start, v_end);
        if (hex_mode)
        {
            for (size_t k = 0; k < island_size; ++k)
                print_byte_hex(a1[island_size - 1 - k]);
            printf("\n\nw island [%d to %d]: ", w_start, w_end);
            for (size_t k = 0; k < island_size; ++k)
                print_byte_hex(a2[island_size - 1 - k]);
        }
        else
        {
            for (size_t k = 0; k < island_size; ++k)
                print_byte(a1[island_size - 1 - k]);
            printf("\n\nw island [%d to %d]: ", w_start, w_end);
            for (size_t k = 0; k < island_size; ++k)
                print_byte(a2[island_size - 1 - k]);
        }
        printf("\n\n");

        free(a1);
        free(a2);
        high = 0;
        prev_i = high_i;
        prev_j = high_j;
    }
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
        perror(filename);
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
        perror("Error reading file");
        free(buffer);
        fclose(f);
        return NULL;
    }

    fclose(f);
    *out_size = size;
    return buffer;
}

void print_help()
{
    printf(
        "Usage: align <file1> <file2> [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -n <#>    Show alignments of <#> islands.\n"
        "  -h        Hex mode.\n"
        "  -ma <#>   Match score (default 1).\n"
        "  -mis <#>  Mismatch score (default -1).\n"
        "  -gp <#>   Gap penalty (default -2).\n"
        "\n"
        "Example:\n"
        "  align fileA.txt fileB.txt -n 5 -h\n");
}

int main(int argc, char *argv[])
{
    int n = 3;
    int hex_mode = 0;

    if (argc == 1)
    {
        print_help();
        return 1;
    }
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
        else if (strcmp(argv[i], "-ma") == 0 && i + 1 < argc)
        {
            MATCH = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-mis") == 0 && i + 1 < argc)
        {
            MISMATCH = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-gp") == 0 && i + 1 < argc)
        {
            GAP_PENALTY = atoi(argv[++i]);
        }
        else
        {
            if (i == 1)
                v = argv[i];
            else if (i == 2)
                w = argv[i];
            else
            {
                printf("Unknown argument: \"%s\"\n", argv[i]);
                print_help();
                return 1;
            }
        }
    }

    v = read_file((const char *)v, &v_len);
    w = read_file((const char *)w, &w_len);

    if (!v || !w)
    {
        print_help();
        return 1;
    }

    if (v_len * w_len > 4000000)
    {
        printf("Alignment exceeds memory limit! Try with smaller files.");
        return;
    }

    smith_waterman();
    // print_sequences();
    // print_table();
    print_solution(n, hex_mode);

    return 0;
}