#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef struct {
    int count;
    size_t size;
    double noise;
    int binsize;
    size_t** data;
    char** label;
} multihist_t;

multihist_t* multihist_init(size_t size, int count) {
    multihist_t* hist = malloc(sizeof(multihist_t));
    hist->count = count;
    hist->size = size;
    hist->data = malloc(count * sizeof(size_t*));
    hist->label = malloc(count * sizeof(char*));
    for(size_t i = 0; i < count; i++) {
        hist->data[i] = calloc(size, sizeof(size_t));
        char label_buffer[64];
        sprintf(label_buffer, "Data %zd", i);
        hist->label[i] = strdup(label_buffer);
    }
    hist->noise = 0;
    hist->binsize = 1;
    return hist;
}

void multihist_set_noise_threshold(multihist_t* hist, double threshold) {
    hist->noise = threshold;
}

void multihist_inc(multihist_t* hist, int bin, size_t value) {
    value /= hist->binsize;
    if(value < hist->size) {
        hist->data[bin][value]++;
    }
}

void multihist_set_binsize(multihist_t* hist, int binsize) {
    hist->binsize = binsize;
}


void multihist_cap(multihist_t* hist, size_t* start_idx, size_t* end_idx) {
    size_t start = 0, end = hist->size - 1, sum = 0;

    for(size_t i = 0; i < hist->size; i++) {
        for(int j = 0; j < hist->count; j++) {
            sum += hist->data[j][i];
        }
    }
    size_t thresh = sum * hist->noise;
    while(start < hist->size) {
        int stop = 0;
        for(int j = 0; j < hist->count; j++) {
            if(hist->data[j][start] > thresh) stop = 1;
        }
        if(stop) break;
        start++;
    }
    while(end > 0) {
        int stop = 0;
        for(int j = 0; j < hist->count; j++) {
            if(hist->data[j][end] > thresh) stop = 1;
        }
        if(stop) break;
        end--;
    }
    *start_idx = start;
    *end_idx = end;
}

void multihist_print(multihist_t* hist, size_t print_width) {
    size_t start, end, max = 0;
    multihist_cap(hist, &start, &end);
    for(size_t i = start; i <= end; i++) {
        size_t s = 0;
        for(int j = 0; j < hist->count; j++) {
            s += hist->data[j][i];
        }
        if(s > max) max = s;
    }
    if(max == 0) max = 1;
    double norm = (double)print_width / max;
    if((ssize_t)end - (ssize_t)start + 2 <= 0) {
        printf(" [ empty ]\n");
        return;
    }
    size_t canvas_size = (print_width + 1) * (end - start + 2);
    char* canvas = malloc(sizeof(char) * canvas_size);
    memset(canvas, ' ', canvas_size);
    char* color = malloc(sizeof(char) * canvas_size);
    memset(color, 0, canvas_size);
    
    size_t y = 0;
    for(size_t i = start; i <= end; i++) {
        for(int j = 0; j < hist->count; j++) {
            if(hist->data[j][i]) {
                canvas[y * (print_width + 1) + (int)(hist->data[j][i] * norm)] = '#';
                color[y * (print_width + 1) + (int)(hist->data[j][i] * norm)] = j + 1;
            }
        }
        y++;
    }

    y = 0;
    int* dither = malloc(print_width * sizeof(int));
    for(size_t i = start; i <= end; i++) {
        printf("%4zd: ", i * hist->binsize);
        int dither_col_count = 0;
        memset(dither, 0, print_width * sizeof(int));
        for(int x = 0; x < print_width; x++) {
            dither[x] = 0;
            for(int j = 0; j < hist->count; j++) {
                int len = (int)(hist->data[j][i] * norm);
                if(len) {
                    for(int d = 0; d < len; d++) {
                        dither[d] |= 1 << j;
                    }
                }
            }
        }
        
        int dither_counter = 0;
        for(int x = 0; x < print_width; x++) {
            int idx = y * (print_width + 1) + x;
            if(canvas[idx] == ' ') {
                int ccol = -1, col = 0;
                for(int j = 0; j < 32; j++) {
                    if((dither[x] >> j) & 1) {
                        ccol++;
                        if(ccol == dither_counter) {
                            col = j + 1;
                            break;
                        }
                    }
                }
                printf("\x1b[3%xm░\x1b[0m", col);
            } else {
                printf("\x1b[3%xm█\x1b[0m", color[idx]); //, canvas[idx]);
            }
            int colors = __builtin_popcount(dither[x]);
            if(colors) dither_counter = (dither_counter + 1) % colors;
        }
        y++;
        printf("\n");
    }
    printf("\n");
    for(int i = 0; i < hist->count; i++) {
        printf(" \x1b[3%xm█\x1b[0m %s    ", i + 1, hist->label[i]);
    }
    printf("\n");
    free(dither);
    free(canvas);
    free(color);
} 


void multihist_set_label(multihist_t* hist, int bin, const char* label) {
    if(bin >= hist->count) return;
    free(hist->label[bin]);
    hist->label[bin] = strdup(label);
}


int multihist_export_csv(multihist_t* hist, const char* filename) {
    FILE* f = fopen(filename, "w");
    if(!f) return 1;
    fprintf(f, "Time");
    for(int i = 0; i < hist->count; i++) {
        fprintf(f, ",%s", hist->label[i]); 
    }
    fprintf(f, "\n");

    size_t start, end, max = 0;
    multihist_cap(hist, &start, &end);
    for(size_t i = start; i <= end; i++) {
        fprintf(f, "%zd", i);
        for(int j = 0; j < hist->count; j++) {
            fprintf(f, ",%zd", hist->data[j][i]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}
