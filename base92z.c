#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// Base92 alphabet: your chosen characters (length 92)
const char mnemonics[93] = 
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "0123456789-+!@#$^&*()_=<>,/'{}[?`~|.\\;:";

const int BASE = 92;

// Find index of character in alphabet
int find_index(char c) {
    for (int i = 0; i < BASE; i++) {
        if (mnemonics[i] == c) return i;
    }
    return -1;
}

// Max raw bytes per encode block: 5 bytes = 40 bits < ceil(log2(92^7)) ~ 40.1 bits
// So encode 5 bytes → up to 7 base92 chars
#define RAW_BLOCK_SIZE 5
#define ENCODED_BLOCK_SIZE 7

// Encode 5 bytes to 7 base92 characters
void encode_block(const uint8_t *in, size_t in_len, char *out) {
    // Treat input as a big integer:
    uint64_t value = 0;
    for (size_t i = 0; i < in_len; i++) {
        value = (value << 8) | in[i];
    }
    // Pad missing bytes with 0 (if in_len < 5)
    for (size_t i = in_len; i < RAW_BLOCK_SIZE; i++) {
        value <<= 8;
    }
    // Convert to base92 digits
    for (int i = ENCODED_BLOCK_SIZE - 1; i >= 0; i--) {
        out[i] = mnemonics[value % BASE];
        value /= BASE;
    }
}

// Decode 7 base92 chars to 5 bytes (some may be padding)
void decode_block(const char *in, uint8_t *out, size_t *out_len) {
    uint64_t value = 0;
    for (int i = 0; i < ENCODED_BLOCK_SIZE; i++) {
        int idx = find_index(in[i]);
        if (idx < 0) {
            fprintf(stderr, "Invalid character '%c' in input\n", in[i]);
            exit(1);
        }
        value = value * BASE + idx;
    }
    // Extract bytes from the big integer (5 bytes)
    for (int i = RAW_BLOCK_SIZE - 1; i >= 0; i--) {
        out[i] = value & 0xFF;
        value >>= 8;
    }
    *out_len = RAW_BLOCK_SIZE;
}

void encode_file(const char* input_path, const char* output_path) {
    FILE* fin = fopen(input_path, "rb");
    if (!fin) { perror("Failed to open input file"); return; }
    FILE* fout = fopen(output_path, "w");
    if (!fout) { perror("Failed to open output file"); fclose(fin); return; }

    // Write file size as 16 hex chars at start (no newline)
    fseek(fin, 0, SEEK_END);
    uint64_t filesize = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    fprintf(fout, "%016" PRIx64, filesize);

    uint8_t buffer[RAW_BLOCK_SIZE];
    char encoded[ENCODED_BLOCK_SIZE];

    size_t read;
    while ((read = fread(buffer, 1, RAW_BLOCK_SIZE, fin)) > 0) {
        // If less than RAW_BLOCK_SIZE read, pad buffer with zeros
        if (read < RAW_BLOCK_SIZE) {
            for (size_t i = read; i < RAW_BLOCK_SIZE; i++) buffer[i] = 0;
        }
        encode_block(buffer, read, encoded);
        fwrite(encoded, 1, ENCODED_BLOCK_SIZE, fout);
    }

    fclose(fin);
    fclose(fout);
}

void decode_file(const char* input_path, const char* output_path) {
    FILE* fin = fopen(input_path, "r");
    if (!fin) { perror("Failed to open input file"); return; }
    FILE* fout = fopen(output_path, "wb");
    if (!fout) { perror("Failed to open output file"); fclose(fin); return; }

    // Read file size header (16 hex chars)
    char filesize_str[17] = {0};
    if (fread(filesize_str, 1, 16, fin) != 16) {
        fprintf(stderr, "Failed to read file size header\n");
        fclose(fin); fclose(fout);
        return;
    }
    uint64_t filesize = 0;
    sscanf(filesize_str, "%" SCNx64, &filesize);

    char encoded[ENCODED_BLOCK_SIZE];
    uint8_t decoded[RAW_BLOCK_SIZE];
    uint64_t bytes_written = 0;

    while (bytes_written < filesize) {
        size_t read_chars = fread(encoded, 1, ENCODED_BLOCK_SIZE, fin);
        if (read_chars < ENCODED_BLOCK_SIZE) {
            fprintf(stderr, "Corrupted encoded file or premature EOF\n");
            break;
        }
        decode_block(encoded, decoded, &read_chars);

        // Write only remaining bytes (handle last block)
        size_t to_write = (filesize - bytes_written) < RAW_BLOCK_SIZE ? (size_t)(filesize - bytes_written) : RAW_BLOCK_SIZE;
        fwrite(decoded, 1, to_write, fout);
        bytes_written += to_write;
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