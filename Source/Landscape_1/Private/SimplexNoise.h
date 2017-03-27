/*
SimplexNoise 1.0.0
-----
DevDad - Afan Olovcic @ www.art-and-code.com - 08/12/2015
This algorithm was originally designed by Ken Perlin, but my code has been
adapted and extended from the implementation written by Stefan Gustavson (stegu@itn.liu.se)
and modified to fit to Unreal Engine 4
* This is a clean, fast, modern and free Perlin Simplex noise function.
* If we change float to double it could be even faster but there is no double type in Blueprint
* All Public Functions are BlueprintCallable so they can be used in every blueprint
From DevDad and Dedicated to you and Unreal Community
Use it free for what ever you want
I only request that you mention me in the credits for your game in the way that feels most appropriate to you.
*/

#pragma once

class LANDSCAPE_1_API USimplexNoise
{
public:
	static void setNoiseSeed(const int32& newSeed);
	static float SimplexNoise2D(float x, float y);
private:
	static unsigned char perm[];
	static float  grad(int hash, float x, float y);
};