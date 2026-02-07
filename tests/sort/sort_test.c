#include "sort.h"
#include "util.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static bool compare_function_callback(void* a, void* b)
{
    char a_char = *(char*)a;
    char b_char = *(char*)b;

    printf("a_char: %c, b_char: %c\n", (unsigned)a_char, (unsigned)b_char);

    return a_char > b_char;
}

void test_sort_already_sorted()
{
    char test_array[26] = "ABCDEFGHIJKLMNOQRSTUVWXYZ";
    SortArgs args = {
        .array = (void**)&test_array,
        .size = 26,
        .compare = compare_function_callback,
    };

    insertion_sort(args);
}

void test_sort_reverse_sorted()
{

}

void test_sort_few_unique_keys()
{

}

void test_sort_random()
{

}

int main(void)
{
    printf("Testing Sort Already Sorted.\n");
    test_sort_already_sorted();
    printf("Testing Sort Reverse Sorted.\n");
    test_sort_reverse_sorted();
    printf("Testing Sort Few Unique Keys.\n");
    test_sort_few_unique_keys();
    printf("Testing Sort Random.\n");
    test_sort_random();

    printf("-------------------------------------------------------------------------------\n");
    printf("Sort Tests Passed :)\n");
    printf("-------------------------------------------------------------------------------\n");

    return 0;
}
