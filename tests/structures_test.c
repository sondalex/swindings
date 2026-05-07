#include "structures.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

void test_in_intlist(void) {
    intlist_t *list = malloc(sizeof(intlist_t));
    TEST_ASSERT_NOT_NULL(list);

    intlist_init(list);
    intlist_append(list, 0);
    intlist_append(list, 2);
    intlist_append(list, 3);

    TEST_ASSERT_TRUE(in_intlist(0, list));
    TEST_ASSERT_TRUE(in_intlist(2, list));
    TEST_ASSERT_TRUE(in_intlist(3, list));
    TEST_ASSERT_FALSE(in_intlist(1, list));

    intlist_free(list);
    free(list);
}

void test_get_segments(void) {
    char *text = "Text, highlighted from here. Not here.";
    intlist_t *positions = malloc(sizeof(intlist_t));
    TEST_ASSERT_NOT_NULL(positions);
    intlist_init(positions);

    intlist_append(positions, 18);
    intlist_append(positions, 19);
    intlist_append(positions, 20);
    intlist_append(positions, 21);
    intlist_append(positions, 22);
    intlist_append(positions, 23);
    intlist_append(positions, 24);
    intlist_append(positions, 25);
    intlist_append(positions, 26);

    segmentlist_t *segments = malloc(sizeof(segmentlist_t));
    TEST_ASSERT_NOT_NULL(segments);
    segmentlist_init(segments);

    size_t n = get_segments(text, positions, segments);

    /* Use the correct assertions for size_t */
    TEST_ASSERT_EQUAL_size_t(3, segments_len(segments));
    TEST_ASSERT_EQUAL_size_t(3, n);

    /* Create expected segments */
    segment_t segment1 = {.start = 0, .end = 18, .highlighted = false};
    segment_t segment2 = {.start = 18, .end = 27, .highlighted = true};
    segment_t segment3 = {.start = 27,
                          .end = strlen(text),
                          .highlighted = false}; // assuming default is false

    /* Compare structs using memory comparison */
    TEST_ASSERT_EQUAL_MEMORY(&segment1, &segments->items[0], sizeof(segment_t));
    TEST_ASSERT_EQUAL_MEMORY(&segment2, &segments->items[1], sizeof(segment_t));
    TEST_ASSERT_EQUAL_MEMORY(&segment3, &segments->items[2], sizeof(segment_t));

    /* Cleanup */
    intlist_free(positions);
    free(positions);

    segmentlist_free(segments); // or whatever your free function is
    free(segments);
}
