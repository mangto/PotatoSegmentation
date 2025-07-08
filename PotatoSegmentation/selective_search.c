#include "image.h"

#include <stdio.h>
#include <math.h>

float pixel_distance(Pixel a, Pixel b) {

	float distance;

	distance = sqrt(
		pow(a.r - b.r, 2) + pow(a.g - b.g, 2) + pow(a.b - b.b, 2)
	);

	return distance;
}