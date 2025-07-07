#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// Your mnemonics (same as before)
const char* mnemonics[64] = {
    "a", "b", "c", "d", "e", "f", "g", "h",
    "i", "j", "k", "l", "m", "n", "o", "p",
    "q", "r", "s", "t", "u", "v", "w", "x",
    "y", "z", "A", "B", "C", "D", "E", "F",
    "G", "H", "I", "J", "K", "L", "M", "N",
    "O", "P", "Q", "R", "S", "T", "U", "V",
    "W", "X", "Y", "Z", "0", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "-", "+"
};

int find_index(const char* word) {
    for (int i = 0; i < 64; i++) {
        if (strcmp(word, mnemonics[i]) == 0) return i;
    }
    return -1;
}

void encode(uint32_t val, int mnemonic_out[6]) {
    uint64_t padded = ((uint64_t)val) << 4; // pad to 36 bits
    for (int i = 0; i < 6; i++) {
        mnemonic_out[i] = (padded >> (36 - 6 * (i + 1))) & 0x3F;
    }
}

uint32_t decode(const int mnemonic_in[6]) {
    uint64_t combined = 0;
    for (int i = 0; i < 6; i++) {
        combined |= ((uint64_t)mnemonic_in[i] & 0x3F) << (36 - 6 * (i + 1));
    }
    return (uint32_t)(combined >> 4);
}

void encode_file(const char* input_path, const char* output_path) {
    FILE* fin = fopen(input_path, "rb");
    if (!fin) {
        perror("Failed to open input file");
        return;
    }

    FILE* fout = fopen(output_path, "w");
    if (!fout) {
        perror("Failed to open output file");
        fclose(fin);
        return;
    }

    // Get input file size
    fseek(fin, 0, SEEK_END);
    uint64_t filesize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    // Write file size as 16 hex chars at start
    //fprintf(fout, "%016" PRIx64 "\n", filesize);
    fprintf(fout, "%016" PRIx64, filesize);

    uint8_t buffer[4];
    int mnemonics_out[6];

    while (!feof(fin)) {
        size_t read = fread(buffer, 1, 4, fin);
        if (read == 0) break;

        // Pad with zeros if less than 4 bytes
        for (size_t i = read; i < 4; i++) buffer[i] = 0;

        uint32_t val = 0;
        for (int i = 0; i < 4; i++) {
            val |= ((uint32_t)buffer[i]) << (8 * (3 - i)); // Big endian
        }

        encode(val, mnemonics_out);

        for (int i = 0; i < 6; i++) {
            fputs(mnemonics[mnemonics_out[i]], fout);
        }
    }

    fclose(fin);
    fclose(fout);
}

void decode_file(const char* input_path, const char* output_path) {
    FILE* fin = fopen(input_path, "r");
    if (!fin) {
        perror("Failed to open input file");
        return;
    }

    FILE* fout = fopen(output_path, "wb");
    if (!fout) {
        perror("Failed to open output file");
        fclose(fin);
        return;
    }

    // Read file size line (16 hex chars + newline)
    char filesize_str[17] = {0};
    size_t bytes_read = fread(filesize_str, 1, 16, fin);
    if (bytes_read != 16) {
        fprintf(stderr, "Failed to read file size header\n");
        fclose(fin);
        fclose(fout);
        return;
    }
    uint64_t filesize = 0;
    sscanf(filesize_str, "%" SCNx64, &filesize);

    char chunk[6 * 2]; // max 2 chars per mnemonic but your mnemonic words are 1-char so 6 chars total
    int mnemonics_in[6];

    uint64_t bytes_written = 0;

    while (bytes_written < filesize) {
        int read_chars = fread(chunk, 1, 6, fin);
        if (read_chars < 6) {
            fprintf(stderr, "Corrupted encoded file or premature EOF\n");
            break;
        }

        // Map chars to indices
        for (int i = 0; i < 6; i++) {
            char c[2] = { chunk[i], '\0' };
            int idx = find_index(c);
            if (idx == -1) {
                fprintf(stderr, "Invalid mnemonic char '%c' in input\n", chunk[i]);
                fclose(fin);
                fclose(fout);
                return;
            }
            mnemonics_in[i] = idx;
        }

        uint32_t val = decode(mnemonics_in);

        // Extract bytes from val (big endian)
        for (int i = 0; i < 4 && bytes_written < filesize; i++) {
            uint8_t b = (val >> (8 * (3 - i))) & 0xFF;
            fputc(b, fout);
            bytes_written++;
        }
    }

    fclose(fin);
    fclose(fout);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage:\n  %s encode <inputfile> <outputfile>\n  %s decode <inputfile> <outputfile>\n", argv[0], argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "encode") == 0) {
        encode_file(argv[2], argv[3]);
        printf("Encoding complete.\n");
    } else if (strcmp(argv[1], "decode") == 0) {
        decode_file(argv[2], argv[3]);
        printf("Decoding complete.\n");
    } else {
        fprintf(stderr, "Unknown command '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}