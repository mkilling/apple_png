#ifndef DYN_ARR_H
#define DYN_ARR_H

/* dynamic array implementation */

typedef struct dyn_arr_t {
    size_t size;
    size_t used;
    char *arr;
} dyn_arr;

dyn_arr *dyn_arr_create(size_t size) {
    dyn_arr *ret = calloc(1, sizeof(dyn_arr));
    if (!ret) {
        return 0;
    }
    ret->size = size;
    ret->arr = malloc(size * sizeof(char));
    if (!ret->arr) {
        free(ret);
        return 0;
    }
    return ret;
}

void dyn_arr_free(dyn_arr *arr) {
    free(arr->arr);
    memset(arr, 0, sizeof(dyn_arr));
    free(arr);
}

void dyn_arr_append(dyn_arr *arr, const char *elements, size_t el_count) {
    while (arr->used + el_count > arr->size) {
        arr->size *= 2;
        arr->arr = realloc(arr->arr, arr->size * sizeof(char));
    }
    memcpy(&arr->arr[arr->used], elements, el_count);
    arr->used += el_count;
}

#endif