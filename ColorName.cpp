#include "pch.h"
#include "ColorName.h"


int ColorName::computeMSE(int pixR, int pixG, int pixB) {
		return (int)(((pixR - r) * (pixR - r) + (pixG - g) * (pixG - g) + (pixB - b)
			* (pixB - b)) / 3);
	}

int ColorName::getR() {
		return r;
	}

int ColorName::getG() {
		return g;
	}

int ColorName::getB() {
		return b;
	}

String^ ColorName::getName() {
		return name;
	}
