#include <stdio.h>
#include <array>
#include <fstream>
#include <iostream>
#include <tuple>
#include <vector>
#include "get_time.h"

using ChipSequence = std::array<int8_t, 1023>;

constexpr std::array<std::tuple<int, int>, 24> satelite_xored_bits = {{
    {2, 6},  {3, 7}, {4, 8}, {5, 9}, {1, 9}, {2, 10}, {1, 8}, {2, 9},
    {3, 10}, {2, 3}, {3, 4}, {5, 6}, {6, 7}, {7, 8},  {8, 9}, {9, 10},
    {1, 4},  {2, 5}, {3, 6}, {4, 7}, {5, 8}, {6, 9},  {1, 3}, {4, 6},
}};

std::ostream &operator<<(std::ostream &s, const ChipSequence &seq)
{
    s << "[";

    for (auto val : seq)
    {
        s << static_cast<int>(val) << ", ";
    }

    s << "]";
    return s;
}

auto read_chip_sequence(std::ifstream &stream) -> ChipSequence
{
    auto chip_sequence = ChipSequence{};

    // read a chip sequence
    int word;
    size_t i = 0;
    while (stream >> word)
    {
        if (i == 1023)
        {
            std::cerr << "input too large";
            exit(1);
        }

        chip_sequence[i] = static_cast<int8_t>(word);
        i++;
    }

    if (i != 1023)
    {
        std::cerr << "input too small (" << i << " out of 1023 chips)";
        exit(1);
    }

    return chip_sequence;
}

auto generate_pseudo_random_sequence(int first_xored_bit, int second_xored_bit)
    -> ChipSequence
{
    auto seq = ChipSequence{};

    int reg1 = 0x3ff;  // 10 × 1
    int reg2 = 0x3ff;  // 10 × 1

    for (size_t i = 0; i < 1023; i++)
    {
        // write value
        seq[i] = (reg1 & 1) ^ ((reg2 >> (10 - first_xored_bit)) & 1) ^
                 ((reg2 >> (10 - second_xored_bit)) & 1);

        // update first register
        int tmp = (reg1 & 1) ^ ((reg1 & 0x80) >> 7);
        reg1 = (reg1 >> 1) | (tmp << 9);

        // update second register
        tmp = (reg2 & 1) ^ ((reg2 & 2) >> 1) ^ ((reg2 & 4) >> 2) ^
              ((reg2 & 0x10) >> 4) ^ ((reg2 & 0x80) >> 7) ^
              ((reg2 & 0x100) >> 8);

        reg2 = (reg2 >> 1) | (tmp << 9);
    }

    return seq;
}

void check_satelite_sequence(int satelite_nr, const ChipSequence &chip_sequence,
                             const ChipSequence &satelite_sequence)
{
    for (size_t offset = 0; offset < 1023; offset++)
    {
        int sum = 0;
        for (size_t i = 0; i < 1023; i++)
        {
            size_t idx = (offset + i) % 1023;

            int a = static_cast<int>(chip_sequence[idx]) *
                    static_cast<int>(satelite_sequence[i]);

            sum += a;
        }

        if (abs(sum) > 200)
        {
            auto bit = sum > 0 ? 1 : 0;
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
        auto file = std::ifstream(argv[1], std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "file not found";
            exit(1);
        }

        auto chip_sequence = read_chip_sequence(file);

        auto time = get_time();
        int satelite_nr = 1;
        for (auto &&bits : satelite_xored_bits)
        {
            auto satelite_sequence = generate_pseudo_random_sequence(
                std::get<0>(bits), std::get<1>(bits));

            check_satelite_sequence(satelite_nr, chip_sequence,
                                    satelite_sequence);

            satelite_nr++;
        }

        std::cerr << "time taken: " << (get_time() - time) << "s";
    }

    return 0;
}