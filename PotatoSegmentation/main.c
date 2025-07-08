#include "stb_image.h"
#include "image.h"
#include "selective_search.h"
#include "utils.h"
#include "test.h"

#include <stdio.h>

int main() {

    image_load_test();
    pixel_distance_test();

    return 0;
}