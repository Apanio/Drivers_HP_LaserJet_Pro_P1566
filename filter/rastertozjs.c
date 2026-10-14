/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * HP LaserJet Pro P1566 CUPS raster filter.
 *
 * macOS cgpdftoraster supplies CUPS Raster. This bridge converts each page to
 * raw monochrome PBM, then lets the GPL foo2zjs encoder produce printer data.
 * No Ghostscript, Homebrew runtime, or Intel translation is needed.
 */
#include <cups/cups.h>
#include <cups/raster.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef FOO2ZJS_PATH
#define FOO2ZJS_PATH "/Library/Printers/foo2zjs-p1566/bin/foo2zjs"
#endif

static void report(const char *message) {
    fprintf(stderr, "ERROR: rastertozjs: %s\n", message);
}

static int same(const char *a, const char *b) {
    return a != NULL && strcmp(a, b) == 0;
}

static const char *option(int count, cups_option_t *options, const char *name) {
    const char *value = cupsGetOption(name, count, options);
    return value != NULL ? value : "";
}

static int paper_code(const char *name) {
    struct map { const char *name; int code; };
    static const struct map papers[] = {
        {"Letter", 1}, {"Legal", 5}, {"Executive", 7}, {"A4", 9},
        {"A5", 11}, {"B5", 13}, {"Env10", 20}, {"EnvDL", 27},
        {"EnvC5", 28}, {"EnvISOB5", 34}, {"EnvMonarch", 37},
        {"Postcard", 43}, {"A6", 70}, {"DoublePostcard", 82},
        {"w558h774", 257}, {"FanfoldGermanLegal", 258},
        {"w522h737", 263}, {"w553h765", 264}
    };
    for (size_t i = 0; i < sizeof(papers)/sizeof(papers[0]); i++)
        if (same(name, papers[i].name)) return papers[i].code;
    return 9; /* A4 */
}

static int media_code(const char *name) {
    struct map { const char *name; int code; };
    static const struct map media[] = {
        {"Standard", 1}, {"Plain", 1}, {"Transparency", 2},
        {"Light", 258}, {"Bond", 260}, {"ThickStock", 261},
        {"Heavy", 262}, {"Rough", 263}, {"Labels", 265},
        {"Envelope", 267}, {"Vellum", 273}, {"Medium", 282},
        {"ExtraHeavy", 283}, {"Color", 512}, {"Letterhead", 513},
        {"Preprinted", 514}, {"Prepunched", 515}, {"Recycled", 516}
    };
    for (size_t i = 0; i < sizeof(media)/sizeof(media[0]); i++)
        if (same(name, media[i].name)) return media[i].code;
    return 1;
}

static int source_code(const char *name) {
    if (same(name, "Upper")) return 1;
    if (same(name, "Lower")) return 2;
    if (same(name, "Middle")) return 3;
    if (same(name, "Manual")) return 4;
    if (same(name, "Envelope")) return 5;
    return 7;
}

static int density_code(const char *name) {
    if (strncmp(name, "Density", 7) == 0 &&
        name[7] >= '1' && name[7] <= '5' && name[8] == '\0')
        return name[7] - '0';
    return 3;
}

static int supported_raster(const cups_page_header2_t *h) {
    if (h->cupsWidth == 0 || h->cupsHeight == 0 ||
        h->cupsWidth > 20000 || h->cupsHeight > 20000 ||
        h->cupsBytesPerLine == 0 || h->cupsBytesPerLine > 80000 ||
        h->HWResolution[0] == 0 || h->HWResolution[1] == 0 ||
        h->cupsColorOrder != CUPS_ORDER_CHUNKED)
        return 0;
    if ((h->cupsColorSpace == CUPS_CSPACE_K ||
         h->cupsColorSpace == CUPS_CSPACE_W) &&
        h->cupsBitsPerColor == 1 && h->cupsBitsPerPixel == 1)
        return h->cupsBytesPerLine >= (h->cupsWidth + 7) / 8;
    if ((h->cupsColorSpace == CUPS_CSPACE_K ||
         h->cupsColorSpace == CUPS_CSPACE_W ||
         h->cupsColorSpace == CUPS_CSPACE_SW) &&
        h->cupsBitsPerColor == 8 && h->cupsBitsPerPixel == 8)
        return h->cupsBytesPerLine >= h->cupsWidth;
    if (h->cupsColorSpace == CUPS_CSPACE_RGB &&
        h->cupsBitsPerColor == 8 && h->cupsBitsPerPixel == 24)
        return h->cupsBytesPerLine >= h->cupsWidth * 3;
    return 0;
}

static void to_pbm_row(const cups_page_header2_t *h,
                       const unsigned char *input, unsigned char *output,
                       unsigned y) {
    static const unsigned char bayer[4][4] = {
        {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}
    };
    size_t row_bytes = (h->cupsWidth + 7) / 8;
    memset(output, 0, row_bytes);
    if (h->cupsBitsPerPixel == 1) {
        for (size_t i = 0; i < row_bytes; i++)
            output[i] = (h->cupsColorSpace == CUPS_CSPACE_K)
                      ? input[i] : (unsigned char)~input[i];
        if (h->cupsWidth & 7)
            output[row_bytes - 1] &= (unsigned char)(0xff << (8 - (h->cupsWidth & 7)));
        return;
    }
    for (unsigned x = 0; x < h->cupsWidth; x++) {
        unsigned black;
        if (h->cupsColorSpace == CUPS_CSPACE_RGB) {
            unsigned r = input[3*x], g = input[3*x+1], b = input[3*x+2];
            black = 255 - ((54*r + 183*g + 19*b) >> 8);
        } else {
            black = input[x];
            if (h->cupsColorSpace != CUPS_CSPACE_K) black = 255 - black;
        }
        if (black > bayer[y & 3][x & 3] * 16 + 8)
            output[x >> 3] |= (unsigned char)(0x80 >> (x & 7));
    }
}

static int write_pages(cups_raster_t *raster, FILE *pbm,
                       cups_page_header2_t *first, int *page_count) {
    cups_page_header2_t h;
    *page_count = 0;
    while (cupsRasterReadHeader2(raster, &h)) {
        if (!supported_raster(&h)) {
            fprintf(stderr, "ERROR: rastertozjs: unsupported raster: %ux%u, %u dpi, "
                    "space %u, %u bits/color, %u bits/pixel, order %u\n",
                    h.cupsWidth, h.cupsHeight, h.HWResolution[0],
                    h.cupsColorSpace, h.cupsBitsPerColor, h.cupsBitsPerPixel,
                    h.cupsColorOrder);
            return 0;
        }
        if (*page_count == 0) {
            *first = h;
        } else if (h.cupsWidth != first->cupsWidth ||
                   h.cupsHeight != first->cupsHeight ||
                   h.HWResolution[0] != first->HWResolution[0] ||
                   h.HWResolution[1] != first->HWResolution[1]) {
            report("mixed page sizes or resolutions in one job are unsupported");
            return 0;
        }
        unsigned char *input = malloc(h.cupsBytesPerLine);
        size_t row_bytes = (h.cupsWidth + 7) / 8;
        unsigned char *output = malloc(row_bytes);
        if (input == NULL || output == NULL) {
            free(input); free(output);
            report("out of memory");
            return 0;
        }
        if (fprintf(pbm, "P4\n%u %u\n", h.cupsWidth, h.cupsHeight) < 0) {
            free(input); free(output);
            report("cannot write PBM header");
            return 0;
        }
        for (unsigned y = 0; y < h.cupsHeight; y++) {
            unsigned got = 0;
            while (got < h.cupsBytesPerLine) {
                unsigned n = cupsRasterReadPixels(raster, input + got,
                                                  h.cupsBytesPerLine - got);
                if (n == 0) {
                    free(input); free(output);
                    report("truncated CUPS raster page");
                    return 0;
                }
                got += n;
            }
            to_pbm_row(&h, input, output, y);
            if (fwrite(output, 1, row_bytes, pbm) != row_bytes) {
                free(input); free(output);
                report("cannot write PBM pixels");
                return 0;
            }
        }
        free(input); free(output);
        (*page_count)++;
        fprintf(stderr, "PAGE: %d 1\n", *page_count);
    }
    return !ferror(pbm);
}

static int encode(FILE *pbm, FILE *zjs, const cups_page_header2_t *h,
                  int paper, int media, int source, int density, int draft) {
    char resolution[40], geometry[40], paper_arg[16], media_arg[16];
    char source_arg[16], density_arg[16];
    snprintf(resolution, sizeof(resolution), "%ux%u",
             h->HWResolution[0], h->HWResolution[1]);
    snprintf(geometry, sizeof(geometry), "%ux%u",
             h->cupsWidth, h->cupsHeight);
    snprintf(paper_arg, sizeof(paper_arg), "%d", paper);
    snprintf(media_arg, sizeof(media_arg), "%d", media);
    snprintf(source_arg, sizeof(source_arg), "%d", source);
    snprintf(density_arg, sizeof(density_arg), "%d", density);
    fflush(pbm);
    rewind(pbm);
    pid_t child = fork();
    if (child < 0) {
        report("cannot start foo2zjs");
        return 0;
    }
    if (child == 0) {
        if (dup2(fileno(pbm), STDIN_FILENO) < 0 ||
            dup2(fileno(zjs), STDOUT_FILENO) < 0)
            _exit(126);
        char *args[] = {
            (char *)FOO2ZJS_PATH, "-P", "-z2", "-L0",
            "-r", resolution, "-g", geometry,
            "-p", paper_arg, "-m", media_arg, "-s", source_arg,
            "-T", density_arg, draft ? "-t" : NULL, NULL
        };
        execv(FOO2ZJS_PATH, args);
        fprintf(stderr, "ERROR: rastertozjs: exec %s: %s\n",
                FOO2ZJS_PATH, strerror(errno));
        _exit(127);
    }
    int status;
    if (waitpid(child, &status, 0) != child ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        report("foo2zjs failed");
        return 0;
    }
    if (fflush(zjs) != 0 || fseek(zjs, 0, SEEK_END) != 0) {
        report("cannot inspect encoded output");
        return 0;
    }
    long size = ftell(zjs);
    if (size < 256 || fseek(zjs, 0, SEEK_SET) != 0) {
        report("encoded output is too short");
        return 0;
    }
    unsigned char header[512];
    size_t header_size = fread(header, 1, sizeof(header), zjs);
    int found_magic = 0;
    for (size_t i = 0; i + 4 <= header_size; i++)
        if (memcmp(header + i, "JZJZ", 4) == 0) found_magic = 1;
    if (!found_magic) {
        report("encoded output is not ZjStream");
        return 0;
    }
    rewind(zjs);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 6 || argc > 7) {
        report("usage: rastertozjs job user title copies options [file]");
        return 1;
    }
    int input_fd = STDIN_FILENO;
    if (argc == 7) {
        input_fd = open(argv[6], O_RDONLY);
        if (input_fd < 0) {
            fprintf(stderr, "ERROR: rastertozjs: cannot open %s: %s\n",
                    argv[6], strerror(errno));
            return 1;
        }
    }
    cups_raster_t *raster = cupsRasterOpen(input_fd, CUPS_RASTER_READ);
    if (raster == NULL) {
        report("cannot read CUPS raster");
        if (input_fd != STDIN_FILENO) close(input_fd);
        return 1;
    }
    cups_option_t *options = NULL;
    int count = cupsParseOptions(argv[5], 0, &options);
    FILE *pbm = tmpfile();
    FILE *zjs = tmpfile();
    if (pbm == NULL || zjs == NULL) {
        report("cannot create temporary file");
        if (pbm) fclose(pbm);
        if (zjs) fclose(zjs);
        cupsFreeOptions(count, options);
        cupsRasterClose(raster);
        if (input_fd != STDIN_FILENO) close(input_fd);
        return 1;
    }
    cups_page_header2_t first = {0};
    int page_count;
    int ok = write_pages(raster, pbm, &first, &page_count);
    if (ok && page_count == 0) {
        report("empty raster job");
        ok = 0;
    }
    const char *paper_name = option(count, options, "PageSize");
    if (*paper_name == '\0') paper_name = first.cupsPageSizeName;
    int draft = same(option(count, options, "Quality"), "draft") ||
                same(option(count, options, "print-quality"), "3");
    if (ok)
        ok = encode(pbm, zjs, &first, paper_code(paper_name),
                    media_code(option(count, options, "MediaType")),
                    source_code(option(count, options, "InputSlot")),
                    density_code(option(count, options, "Density")), draft);
    if (ok) {
        unsigned char buffer[65536];
        size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), zjs)) > 0)
            if (fwrite(buffer, 1, n, stdout) != n) { ok = 0; break; }
        if (ferror(zjs) || fflush(stdout) != 0) ok = 0;
        if (!ok) report("cannot send ZjStream to CUPS");
    }
    fclose(zjs);
    fclose(pbm);
    cupsFreeOptions(count, options);
    cupsRasterClose(raster);
    if (input_fd != STDIN_FILENO) close(input_fd);
    return ok ? 0 : 1;
}
