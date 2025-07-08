#ifndef __IMAGE__PROCESS_H__
#define __IMAGE__PROCESS_H__

#include "image.h"
#include "matrix.h"

#include <stdio.h>

void apply_kernel(Image* image, Matrix* kernel);

#endif // !__IMAGE__PROCESS_H__
