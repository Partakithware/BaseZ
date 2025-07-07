# BaseZ
BaseZ64, BaseZ74, and BaseZ92
A set of raw C utilities for encoding and decoding binary files using custom base-encoding schemes (64, 74, 92-character sets).

Each tool is written in pure C (no dependencies beyond the C standard library) and exposes a simple CLI via main().

Build:

gcc -o base64z base64z.c

gcc -o base74z base74z.c

gcc -o base92z base92z.c

Use:

Encode-- file/output-name

./base64z encode file.zip encoded64.txt

./base74z encode file.zip encoded74.txt

./base92z encode file.zip encoded92.txt

Decode-- encoded-file/output-name

./base64z decode encoded64.txt file64.zip

./base74z decode encoded74.txt file74.zip

./base92z decode encoded92.txt file92.zip





