
//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
};

cbuffer Constants
{
	float viewportWidth;
	float viewportHeight;
	float time;
};
//x

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader - input in integer pixel coords, outputs color in [0..1]
////////////////////////////////////////////////////////////////////////////////

float4 main(PixelInputType input) : SV_TARGET
	{
	//note y = 1-inputposition.y/h to flip vertically (GLSL coordinate incompatibility)
	float3 pixcoord = float3(input.position.x/viewportWidth, 1-(input.position.y/viewportHeight), 1) - 0.5;
	float3 outcolor = pixcoord;//reinterpret pixcoord as the background color, in case ray doesn't hit anything.
	float3 grassColor = float3(1, 2.0, 0);	//terrain's green color
	float3 earthColor = float3(2.0, 1.0, 0);//terrain brown color
	float3 camPos = float3(0.3, 0.0, time);
	float camSpeed = 0.3;		//has to be 0.3 to keep camera path over sinusoidal terrain
	float camAmplitude = 3.0;	//amount of vertical camera bob
	camPos.y = 2.5 + camAmplitude * cos(camSpeed*time);	//camera vertical bobbing

	float castDepth = 18.0;		//this is essentially how far back the far plane is was 9
	float rayMarchStep = 0.01;
	float cellScale = 0.2;		//size of the cells (0.2)
	float3 samplePos = camPos;	//world space sample position.
	for (float i = 0.0; i < castDepth; i += 0.01) //raycast??
		{
		samplePos += pixcoord * i * rayMarchStep;	//pixcoord is a vector from eye point through pixel.  we take steps of increasing length (rayMarchStep*i where i increases in each iter) along it in worldspace (camPos add!) 
		//separate position into a cell coordinate and a coordinate inside the cell
		float3 f = frac(samplePos);		//sample position relative to integer cell.
		float3 p = floor(samplePos) * cellScale;	//integer cell coord
		float height = cos(p.z) + sin(p.x) - 1;	//altitude of the world at our sample point
		if (height > p.y)	//is our sample point below the terrain here?
			{
			//sinusoidal squiggle
			float squiggleFrequency = 20.0;
			float cosarg = (samplePos.x + samplePos.z) * squiggleFrequency;
			float squiggleY = 0.04 * cos(cosarg) + 0.8;

			if (f.y > squiggleY)		//is the sample's per-cell Y coord over a sinusoidal line drawn on the side of the cube, separating earth from grass?
				outcolor = grassColor;	//green color (top of cube)
			else
				outcolor = earthColor * f.y;	//brown color (side of cube) - f.y creates a per-cell gradient, so that cell top is bright and cell bottom is dark.

			outcolor = outcolor / i;	//moduleate color with depth, stuff is darker when its further away.
			/*
			//lighting
			float3 normal;
			
			if (f.y > 0.9)
				normal = float3(0.0, 1.0, 0.0);
			else if (f.x > 0.9)
				normal = float3(1.0, 0.0, 0.0);
			else if (f.x < 0.1)
				normal = float3(-1.0, 0.0, 0.0);
			else if (f.z > 0.9)
				normal = float3(0.0, 0.0, -1.0);
			else if (f.z < 0.1)
				normal = float3(0.0, 0.0, 1.0);
			else
				normal = float3(0.0, 0.0, 0.0);	//debug

			//normalize odd normals here

			float3 lightdir = float3(0.3, 0.3, 0.3);

			float diffuse = dot(normal, lightdir);

			float3 reflection = normalize(normal - lightdir);
			float3 eyedir = float3(0.0, 0.0, 1.0);

			float specular = pow(saturate(dot(reflection, eyedir)), 2);

			outcolor = outcolor * diffuse + specular;
			*/

			break;
			}
		}
	return float4(outcolor.x, outcolor.y, outcolor.z, 1);
	}