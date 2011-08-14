// Copyright (c) 2011 Johannes Baiter <johannes.baiter@gmail.com>
// Based on C# code by Kawa <http://helmet.kafuka.org/thepile/Wav2msu>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef WIN32
    #include <io.h>
    #include <fcntl.h>
#endif

void print_usage();
void print_help();
int32_t validate(FILE*);

void print_usage() {
    fprintf(stderr, "Usage: wav2msu [-o outfile] [-l looppoint] [-i introfile] FILE.wav\n");
}

void print_help() {
	printf("wav2msu 0.1\n\n"
               "Usage: wav2msu [-o outfile] [-l looppoint] [-i introfile] FILE.wav\n"
               "Converts wave-files to a MSU1-compatible format.\n"
               "Input is required to be a RIFF WAVE file in 16bit, 44.1kHz, 2ch PCM format.\n"
               "Set filename to \'-\' to read from stdin.\n\n"
               "Arguments:\n"
               "  -i <file.wav>            Put <file.wav> before the main input file.\n"
               "  -l <looppoint>           Set sample (relative to beginning of input file)\n"
               "                           from which to loop, decimal or hexadecimal (0xabcd)\n"
               "  -o <outfile.pcm>         Outputs to given filename, default is stdout\n"
               "  -h                       Print this help\n"
        );
}

// Checks a Wavefile for compliance with the MSU1-specifications[1]
//   [1] http://board.byuu.org/viewtopic.php?f=16&t=947
int32_t validate(FILE *ifp) {
    int riff_header;
    fread(&riff_header, sizeof(riff_header), 1, ifp);
    //  'RIFF' = little-endian
    if (riff_header != 0x46464952) {
        fprintf(stderr, "wav2msu: Incorrect header: Invalid format or endianness\n");
	fprintf(stderr, "         Value was: 0x%x\n", riff_header);
        return -1;
    }

    // stdin doesn't support fseek
    if (ifp == stdin) {
        int i = 0;
        while (++i <= 16) {
            getc(ifp);
        }
    } else
        fseek(ifp, 20, SEEK_SET);

    // Format has to be PCM (=1)
    int16_t format;
    fread(&format, sizeof(format), 1, ifp);
    if (format != 1) {
        fprintf(stderr, "wav2msu: Not in PCM format! (format was: %d)\n", format);
        return -1;
    }

    // The data needs to be in stereo format
    int16_t channels;
    fread(&channels, sizeof(channels), 1, ifp);
    int32_t sample_rate;
    // Sample Rate has to be 44.1kHz
    fread(&sample_rate, sizeof(sample_rate), 1, ifp);
    if (ifp == stdin) {
        int i = 0;
        while (++i <= 6) {
            getc(ifp);
        }
    } else
        fseek(ifp, 34, SEEK_SET);
    int16_t bits_per_sample;
    // We need 16bps
    fread(&bits_per_sample, sizeof(bits_per_sample), 1, ifp);
    if (channels != 2 || sample_rate != 44100 || bits_per_sample != 16) {
        fprintf(stderr, "wav2msu: Not in 16bit 44.1kHz stereo!\n");
        fprintf(stderr, "         Got instead: %dbit, %dHz, %dch\n", bits_per_sample, sample_rate, channels);
        return -1;
    }

    int32_t data_header;
    // 'DATA'
    fread(&data_header, sizeof(data_header), 1, ifp);
    if (data_header != 0x61746164) {
        fprintf(stderr, "wav2msu: Sample data not where expected!\n");
        return -1;
    }

    int32_t data_size;
    fread(&data_size, sizeof(data_size), 1, ifp);
    return data_size;
}

int main(int argc, char *argv[]) {
    FILE *introfile, *infile;
    FILE *outfile = stdout;
    int intro_flag = 0;
    int32_t intro_size, data_size;
    long loop_point = 0;

    // Disable libc error printing
    opterr = 0;

    int opt;
    char *p;
    while ((opt = getopt(argc, argv, "ho:l:i:")) != -1)
        switch (opt) {
            case 'h':
                print_help();
                exit(0);
            case 'o':
                if ((outfile = fopen(optarg, "wb")) == NULL) {
                    fprintf(stderr, "wav2msu: can't open %s\n", optarg);
                    exit(1);
                }
                break;
            case 'l':
                loop_point = strtol(optarg, &p, 0);
                break;
            case 'i':
                intro_flag = 1;
                if ((introfile = fopen(optarg, "rb")) == NULL) {
                    fprintf(stderr, "wav2msu: can't open %s\n", optarg);
                    exit(1);
                }
                break;
            case '?':
                if (optopt == 'l' || optopt== 'i')
                    fprintf(stderr, "wav2msu: Option -%c requires an argument\n", optopt);
                else
                    fprintf(stderr, "wav2msu: Unknown option -%c\n", optopt);
                print_usage();
                exit(1);
                break;
            default:
                exit(1);
                break;
        }
    if (optind == argc) {
        print_help();
        exit(0);
    } else if (argc - optind > 1) {
        fprintf(stderr, "Too many input files.\n");
        print_usage();
        exit(1);
    } else {
        if (*argv[argc-1] == '-') {
            fprintf(stderr, "Reading from stdin.\n");
            infile = stdin;
            // We need to switch stdin to binary mode, or else we run
            // into problems under Windows
            #ifdef WIN32
                _setmode(fileno(stdin), _O_BINARY);
            #endif
        } else if ((infile = fopen(argv[argc-1], "rb")) == NULL) {
            fprintf(stderr, "wav2msu: can't open %s\n", argv[argc-1]);
            exit(1);
        }
    }


    if (intro_flag == 1) {
        intro_size = validate(introfile);
        loop_point += intro_size/4;
        if (intro_size == -1) {
            fclose(introfile);
            fclose(outfile);
            fclose(infile);
            fprintf(stderr, "wav2msu: Intro file did not validate.\n");
            exit(1);
        }
    }

    data_size = validate(infile);
    if (data_size == -1) {
        fclose(outfile);
        fclose(infile);
        fprintf(stderr, "wav2msu: Input WAV data did not validate.\n");
        exit(1);
    }

    loop_point = (int32_t) loop_point;
    fprintf(outfile, "MSU1");
    fwrite(&loop_point, sizeof(loop_point), 1, outfile);
    if (intro_flag == 1) {
        int c;
        while ((c = getc(introfile)) != EOF)
            putc(c, outfile);
    }
    int c;
    while ((c = getc(infile)) != EOF)
        putc(c, outfile);
    fclose(outfile);
    fclose(infile);

    return 0;
}
