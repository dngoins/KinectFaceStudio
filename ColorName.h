// Color matching taken from:
// https://gist.github.com/XiaoxiaoLi/8031146
// re- implemented for ref C++ class

#pragma once

#include "pch.h"

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

ref class ColorName sealed
{
public:
	ColorName(String^ name, int r, int g, int b) :r(r), g(g), b(b), name(name) {}
	int computeMSE(int pixR, int pixG, int pixB);
	int ColorName::getR();
	int ColorName::getG();
	int ColorName::getB();
	String^ ColorName::getName();

private:
	String^ name;
	int r, g, b;

};

