#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "get_time.h"
#include <immintrin.h>

// positive or negative
typedef char Chip;
// 0 or 0xff (=> 1)
typedef char Bit;

#define SATELITE_COUNT 24
#define CHIP_SEQ_LEN 1023

int satelite_xored_bits[SATELITE_COUNT][2] = {
    {2, 6},  {3, 7}, {4, 8}, {5, 9}, {1, 9}, {2, 10}, {1, 8}, {2, 9},
    {3, 10}, {2, 3}, {3, 4}, {5, 6}, {6, 7}, {7, 8},  {8, 9}, {9, 10},
    {1, 4},  {2, 5}, {3, 6}, {4, 7}, {5, 8}, {6, 9},  {1, 3}, {4, 6},
};

void print256(__m256i value)
{
    const size_t n = 32;
    char buffer[n];
    _mm256_storeu_si256((__m256i*)buffer, value);
    for (size_t i = 0; i < n; i++)
        printf("%d, ", buffer[i]);

    printf("\n");
}

void print_chip_sequence(Chip *chip_seq)
{
    printf("[");

    for (size_t i = 0; i < CHIP_SEQ_LEN; i++)
    {
        printf("%d, ", chip_seq[i]);
    }

    printf("]\n");
}

void read_chip_sequence(FILE *f, Chip *seq_out)
{
    int word;
    Chip *it = seq_out;
    Chip *end = seq_out + CHIP_SEQ_LEN;

    while (fscanf(f, "%d", &word) != EOF)
    {
        if (it == end)
        {
            fprintf(stderr, "error: input too large\n");
            exit(1);
        }

        *(it++) = word;
    }

    if (it != end)
    {
        fprintf(stderr, "error: input too short\n");
        exit(1);
    }
}

void generate_pseudo_random_sequence(int first_xored_bit, int second_xored_bit,
                                     Bit *seq_out)
{
    int reg1 = 0x3ff;  // 10 × 1
    int reg2 = 0x3ff;  // 10 × 1

    Bit *it = seq_out;

    for (size_t i = 0; i < 1023; i++)
    {
        // write value
        int out_bool = (reg1 & 1) ^ ((reg2 >> (10 - first_xored_bit)) & 1) ^
                  ((reg2 >> (10 - second_xored_bit)) & 1);

        *(it++) = out_bool == 1 ? 0xff : 0;

        // update first register
        int tmp = (reg1 & 1) ^ ((reg1 & 0x80) >> 7);
        reg1 = (reg1 >> 1) | (tmp << 9);

        // update second register
        tmp = (reg2 & 1) ^ ((reg2 & 2) >> 1) ^ ((reg2 & 4) >> 2) ^
              ((reg2 & 0x10) >> 4) ^ ((reg2 & 0x80) >> 7) ^
              ((reg2 & 0x100) >> 8);

        reg2 = (reg2 >> 1) | (tmp << 9);
    }
}

void check_satelite_sequence(int satelite_nr, const Chip *chip_seq,
                             const Bit *sat_seq)
{
    for (size_t offset = 0; offset < 1023; offset++)
    {
        __m256i sum256 = _mm256_setzero_si256();
        int sum = 0;

        for (size_t i = 0; i < 1023; i += 32) {
            size_t idx = offset + i;

            __m256i a = _mm256_lddqu_si256(chip_seq + idx);
            __m256i b = _mm256_lddqu_si256(sat_seq + i);
            __m256i res = _mm256_and_si256(a, b);

            sum256 = _mm256_add_epi8(sum256, res);
        }

        char buf[32];
        _mm256_store_si256(buf, sum256);

        for (int i = 0; i < 32; i++) {
            sum += (int)buf[i];
        }

        if (abs(sum) > 200)
        {
            int bit = sum > 0 ? 1 : 0;
            printf("Satellite %2d has sent bit %d (delta = %jd)\n", satelite_nr,
                bit, offset);
            break;
        }
    }
}

int main(int argc, const char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
    }
    else
    {
        FILE *f = fopen(argv[1], "r");

        if (f == NULL)
        {
            fprintf(stderr, "error reading file");
            exit(1);
        }


        Chip chip_seq[CHIP_SEQ_LEN * 2];

        read_chip_sequence(f, chip_seq);

        // duplicate to save modulo operation
        memcpy(chip_seq + 1023, chip_seq, CHIP_SEQ_LEN);

        double time = get_time();

        for (int sat_idx = 0; sat_idx < SATELITE_COUNT; sat_idx++)
        {
            Bit sat_seq[CHIP_SEQ_LEN];

            generate_pseudo_random_sequence(satelite_xored_bits[sat_idx][0],
                                            satelite_xored_bits[sat_idx][1],
                                            sat_seq);

            check_satelite_sequence(sat_idx + 1, chip_seq, sat_seq);
        }

        fprintf(stderr, "time taken: %fs", (get_time() - time));
    }

    return 0;
}