
//note: uniformly distributed, normalized rand, [0;1[ (from: https://www.shadertoy.com/view/4ssXRX ) 
float nrand(float2 n)
{
	return frac(sin(dot(n.xy, float2(12.9898, 78.233))) * 43758.5453);
}


float hash(in float n)
{
	return frac(sin(n) * 43758.5453);
}

//this is needed because hlsl fmod doesn't do this.
float mod(float x, float y)
{
	return x - y * floor(x / y);
}

//this is needed because hlsl fmod doesn't do this.
float3 mod3(float3 x, float3 y)
{
	return x - y * floor(x / y);
}


bool getMaterialColor(int i, float2 coord, out float3 color)
{
	// 16x16 tex
	float2 uv = floor(coord);

	//	 mat = 1; // ground
	//	 mat = 2,3 //dirt
	//	 mat = 7; // treetrunk
	//	 mat = 8; // leaves
	//   mat = 9; // ???
	//	 mat = 10; // clouds    
	//	 else: stone

	float n = uv.x + uv.y * 347.0 + 4321.0 * float(i);
	float h = hash(n);

	float br = 1. - h * (96. / 255.);
	color = float3(150. / 255., 108. / 255., 74. / 255.); // 0x966C4A;

	float xm1 = mod((uv.x * uv.x * 3. + uv.x * 81.) / 4., 4.);

	if (i == 1) {
		if (uv.y < (xm1 + 18.)) {
			color = float3(106. / 255., 170. / 255., 64. / 255.); // 0x6AAA40;
		}
		else if (uv.y < (xm1 + 19.)) {
			br = br * (2. / 3.);
		}
	}
	if (i == 4) {
		color = float3(127. / 255., 127. / 255., 127. / 255.); // 0x7F7F7F;
	}
	if (i == 7) {
		color = float3(103. / 255., 82. / 255., 49. / 255.); // 0x675231;
		if (h < 0.5) {
			br = br * (1.5 - mod(uv.x, 2.));
		}
	}
	if (i == 9) {
		color = float3(64. / 255., 64. / 255., 255. / 255.); // 0x4040ff;
	}
	if (i == 8) {
		color = float3(80. / 255., 217. / 255., 55. / 255.); // 0x50D937;
		if (h < 0.5) {
			return false;
		}
	}
	if (i == 10) {
		color = float3(0.65, 0.68, 0.7) * 1.35;
		br = 1.;
	}
	color *= br;
	//color = color * color; //undo suspected baked gamma like we would do for a texture map (square is inverse of 2.0 gamma)
	color = //pow(
		saturate(color);
	//, float3(2.2, 2.2, 2.2));

	return true;
}


float sphIntersect(float3 ro, float3 rd, float4 sph)
{
	float r2 = (sph.w) * (sph.w);
	float3 ros = sph.xyz - ro;
	float d1 = dot(ros, rd);

	float d2 = dot(ros, ros) - r2;
	if (d2 > d1 * d1) return -1.0;
	return d1 - sqrt(d1 * d1 - d2);
}

//trivial 8 bit encoding:
/*
float3 decodeLightmap(uint rgbIn)
{
	float3 color;
	color.r = (rgbIn & 0xff) * (1.0 / 255.0);
	color.g = ((rgbIn >> 8) & 0xff) * (1.0 / 255.0);
	color.b = ((rgbIn >> 16) & 0xff) * (1.0 / 255.0);
	return color;
}

uint encodeLightmap(float3 color)
{
	float3 s = saturate(color);	//otherwise below will overflow.
	return ((uint)(s.r * 255.0)) | (((uint)(s.g * 255.0)) << 8) | (((uint)(s.b * 255.0)) << 16);
}
*/
//r11+g11+b10 encoding

//Trying to compress the (HDR!!) lightmap keeps causing issues.  Forget it for now!
/*
float3 decodeLightmap(uint rgbIn)
{
	float3 color;
	color.r = (rgbIn & 0x7ff) * (1.0 / 2047.0);
	color.g = ((rgbIn >> 11) & 0x7ff) * (1.0 / 2047.0);
	color.b = ((rgbIn >> 22)) * (1.0 / 1024.0);
	return color;
}

uint encodeLightmap(float3 color)
{
	float3 s = saturate(color);	//otherwise below will overflow.
	return ((uint)(s.r * 2047.0)) | (((uint)(s.g * 2047.0)) << 11) | (((uint)(s.b * 1024.0)) << 22);
}
*/