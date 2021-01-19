#include <stdio.h>
#include "colors.h"

int main (void) {

	int red   = 0;
	int green = 7;
	int blue  = 4;
	
	int math = blue * 64 + green * 8 + red;
	math = (blue << 6) + (green << 3) + red;

	printf("%s\n", colors[math]);

	return 0;
}

