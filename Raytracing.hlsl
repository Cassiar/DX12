
// === Defines ===

#define PI 3.141592654f

// === Structs ===

// Layout of data in the vertex buffer
struct Vertex
{
    float3 localPosition	: POSITION;
    float2 uv				: TEXCOORD;
    float3 normal			: NORMAL;
    float3 tangent			: TANGENT;
};
static const uint VertexSizeInBytes = 11 * 4; // 11 floats total per vertex * 4 bytes each


// Payload for rays (data that is "sent along" with each ray during raytrace)
// Note: This should be as small as possible
struct RayPayload
{
	float3 color;
	uint recursionDepth;
	uint rayPerPixelIndex;
	bool inShadow;
};

//used to determine if a point is in shadow
//only needs bool to track that info
//struct ShadowRayPayload
//{
//	bool inShadow;
//};

// Note: We'll be using the built-in BuiltInTriangleIntersectionAttributes struct
// for triangle attributes, so no need to define our own.  It contains a single float2.

//helper functions for pathtracing
//from from Chris's Real-Time Pathtracing Basics lecture
float Rand(float2 uv) {
	//function borrowed from https://thebookofshaders.com/10/
	return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

// === Constant buffers ===

cbuffer SceneData : register(b0)
{
	matrix inverseViewProjection;
	float3 cameraPosition;
	uint raysPerPixel;
	uint maxRecursion;
	float3 lightSourcePos;
	bool inShadow;
};


// Ensure this matches C++ buffer struct define!
#define MAX_INSTANCES_PER_BLAS 100
cbuffer ObjectData : register(b1)
{
	float4 entityColor[MAX_INSTANCES_PER_BLAS];
};


// === Resources ===

// Output UAV 
RWTexture2D<float4> OutputColor				: register(u0);

// The actual scene we want to trace through (a TLAS)
RaytracingAccelerationStructure SceneTLAS	: register(t0);

// Geometry buffers
ByteAddressBuffer IndexBuffer        		: register(t1);
ByteAddressBuffer VertexBuffer				: register(t2);


// === Helpers ===

// Loads the indices of the specified triangle from the index buffer
uint3 LoadIndices(uint triangleIndex)
{
	// What is the start index of this triangle's indices?
	uint indicesStart = triangleIndex * 3;

	// Adjust by the byte size before loading
	return IndexBuffer.Load3(indicesStart * 4); // 4 bytes per index
}

// Barycentric interpolation of data from the triangle's vertices
Vertex InterpolateVertices(uint triangleIndex, float3 barycentricData)
{
	// Grab the indices
	uint3 indices = LoadIndices(triangleIndex);

	// Set up the final vertex
	Vertex vert;
	vert.localPosition = float3(0, 0, 0);
	vert.uv = float2(0, 0);
	vert.normal = float3(0, 0, 0);
	vert.tangent = float3(0, 0, 0);

	// Loop through the barycentric data and interpolate
	for (uint i = 0; i < 3; i++)
	{
		// Get the index of the first piece of data for this vertex
		uint dataIndex = indices[i] * VertexSizeInBytes;

		// Grab the position and offset
		vert.localPosition += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// UV
		vert.uv += asfloat(VertexBuffer.Load2(dataIndex)) * barycentricData[i];
		dataIndex += 2 * 4; // 2 floats * 4 bytes per float

		// Normal
		vert.normal += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
		dataIndex += 3 * 4; // 3 floats * 4 bytes per float

		// Tangent (no offset at the end, since we start over after looping)
		vert.tangent += asfloat(VertexBuffer.Load3(dataIndex)) * barycentricData[i];
	}

	// Final interpolated vertex data is ready
	return vert;
}

//helper function to calculate barycentric data and vertex data
Vertex GetHitDetails(uint triangleIndex, BuiltInTriangleIntersectionAttributes hitAttributes) {
	// Calculate the barycentric data for vertex interpolation
	float3 barycentricData = float3(
		1.0f - hitAttributes.barycentrics.x - hitAttributes.barycentrics.y,
		hitAttributes.barycentrics.x,
		hitAttributes.barycentrics.y);

	// Get the interpolated vertex data
	return InterpolateVertices(triangleIndex, barycentricData);
}

// Calculates an origin and direction from the camera fpr specific pixel indices
void CalcRayFromCamera(float2 rayIndices, out float3 origin, out float3 direction)
{
	// Offset to the middle of the pixel
	float2 pixel = rayIndices + 0.5f;
	float2 screenPos = pixel / DispatchRaysDimensions().xy * 2.0f - 1.0f;
	screenPos.y = -screenPos.y;

	// Unproject the coords
	float4 worldPos = mul(inverseViewProjection, float4(screenPos, 0, 1));
	worldPos.xyz /= worldPos.w;

	// Set up the outputs
	origin = cameraPosition.xyz;
	direction = normalize(worldPos.xyz - origin);
}

//further helper funcs to generate random
float2 Rand2(float2 uv) {
	float x = Rand(uv);
	float y = sqrt(1 - x * x);
	return float2(x, y);
}

float3 Rand3(float2 uv) {
	return float3(Rand2(uv), Rand(uv.yx));
}

float3 RandomVector(float u0, float u1) {
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * PI * u1;

	float x = b * cos(phi);
	float y = b * sin(phi);
	float z = a;

	return float3(x, y, z);
}

float3 RandomCosineWeightedHemisphere(float u0, float u1, float3 unitNormal)
{
	float a = u0 * 2 - 1;
	float b = sqrt(1 - a * a);
	float phi = 2.0f * PI * u1;
	float x = unitNormal.x + b * cos(phi);
	float y = unitNormal.y + b * sin(phi);
	float z = unitNormal.z + a;
	return float3(x, y, z);
}

//helper funcs for refaction
// Fresnel approximation
float FresnelSchlick(float NdotV, float indexOfRefraction)
{
	float r0 = pow((1.0f - indexOfRefraction) / (1.0f + indexOfRefraction), 2.0f);
	return r0 + (1.0f - r0) * pow(1 - NdotV, 5.0f);
}

// Refraction function that returns a bool depending on result
bool TryRefract(float3 incident, float3 normal, float ior, out float3 refr)
{
	float NdotI = dot(normal, incident);
	float k = 1.0f - ior * ior * (1.0f - NdotI * NdotI);

	if (k < 0.0f)
	{
		refr = float3(0, 0, 0);
		return false;
	}

	refr = ior * incident - (ior * NdotI + sqrt(k)) * normal;
	return true;
}

// === Shaders ===

// Ray generation shader - Launched once for each ray we want to generate
// (which is generally once per pixel of our output texture)
[shader("raygeneration")]
void RayGen()
{
	// Get the ray indices
	uint2 rayIndices = DispatchRaysIndex().xy;
	float3 totalColor = float3(0, 0, 0);

	for (uint r = 0; r < raysPerPixel; r++) {
		//move ray slightly off from pixel 
		//so not all are going through the same spot
		float2 adjustedIndices = (float2)rayIndices;
		adjustedIndices += Rand((float)r / raysPerPixel);
		
		// Calculate the ray data
		float3 rayOrigin;
		float3 rayDirection;
		CalcRayFromCamera(adjustedIndices, rayOrigin, rayDirection);

		// Set up final ray description
		RayDesc ray;
		ray.Origin = rayOrigin;
		ray.Direction = rayDirection;
		ray.TMin = 0.0001f;
		ray.TMax = 1000.0f;

		// Set up the payload for the ray
		// This initializes the struct to all zeros
		RayPayload payload;
		payload.color = float3(1, 1, 1);
		payload.recursionDepth = 0;
		payload.rayPerPixelIndex = r;

		// Perform the ray trace for this ray
		TraceRay(
			SceneTLAS,
			RAY_FLAG_NONE,
			0xFF,
			0,
			0,
			0,
			ray,
			payload);

		totalColor += payload.color;
	}

	//average total color
	totalColor /= raysPerPixel;

	// Set the final color of the buffer (gamma corrected)
	OutputColor[rayIndices] = float4(pow(totalColor, 1.0f / 2.2f), 1);
}


// Miss shader - What happens if the ray doesn't hit anything?
[shader("miss")]
void Miss(inout RayPayload payload)
{
	//if we're in shadow (so we just launched the shadow
	//check ray) then we set it to false and return
	//if (payload.inShadow) {
	//	payload.inShadow = false;
	//	return;
	//}
	// Hemispheric gradient
	float3 upColor = float3(0.3f, 0.5f, 0.95f);
	float3 downColor = float3(1, 1, 1);

	// Interpolate based on the direction of the ray
	float interpolation = dot(normalize(WorldRayDirection()), float3(0, 1, 0)) * 0.5f + 0.5f;
	//multiple because we have multiple bounces
	payload.color *= lerp(downColor, upColor, interpolation);
}

[shader("miss")]
void MissShadow(inout RayPayload payload : SV_RayPayload) {
	payload.inShadow = false;
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHit(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	//exit early if we've hit max recursion
	if (payload.recursionDepth >= maxRecursion) {
		payload.color = float3(0, 0, 0);
		return; //don't forget this CASSIAR!!!
	}

	float3 worldOrigin = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());

	RayDesc shadowRay;
	shadowRay.Origin = worldOrigin;
	shadowRay.Direction = normalize(lightSourcePos - worldOrigin);
	shadowRay.TMin = 0.0001f;
	shadowRay.TMax = distance(worldOrigin, lightSourcePos);

	//shadow ray
	RayPayload shadowPayload;
	shadowPayload.inShadow = true;

	TraceRay(SceneTLAS,
		RAY_FLAG_FORCE_OPAQUE //these flag mean we stop when we get any hit
		| RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH //and don't execute the hit shader
		| RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		0,
		1,//skip first miss shader and use second
		shadowRay,
		shadowPayload);

	if (shadowPayload.inShadow) {
		payload.color = float3(0, 0, 0);
		return;
	}

	// we've hit something so update color
	payload.color *= entityColor[InstanceID()].rgb;

	// Grab the index of the triangle we hit
	//and pass to helper func to get Vertex details
	Vertex hit = GetHitDetails(PrimitiveIndex(), hitAttributes);

	//calculate normal in world space
	float3 normal_WS = normalize(mul(hit.normal, (float3x3)ObjectToWorld4x3()));

	//get a unique rng value to offset this ray from other from same pixel
	float2 uv = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	float2 rng = Rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	//lerp between perfect reflection and random bounce based on roughness
	float3 refl = reflect(WorldRayDirection(), normal_WS);
	float3 randomBounce = RandomCosineWeightedHemisphere(Rand(rng), Rand(rng.yx), normal_WS);
	//use alpha channel as roughness
	float3 dir = normalize(lerp(refl, randomBounce, saturate(pow(entityColor[InstanceID()].a,2))));

	RayDesc ray;
	ray.Origin = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
	ray.Direction = dir;// reflect(WorldRayDirection(), normal_WS);
	ray.TMin = 0.0001f;
	ray.TMax = 1000.0f;

	//increase recursion depth
	payload.recursionDepth++;

	TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, 0, 0, 0, //mask and offsets
		ray,
		payload);
}

// Closest hit shader - Runs when a ray hits the closest surface
[shader("closesthit")]
void ClosestHitTransparent(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes)
{
	//exit early if we've hit max recursion
	if (payload.recursionDepth >= maxRecursion) {
		payload.color = float3(0, 0, 0);
		return; //don't forget this CASSIAR!!!
	}

	// we've hit something so update color
	payload.color *= entityColor[InstanceID()].rgb;

	// Grab the index of the triangle we hit
	//and pass to helper func to get Vertex details
	Vertex hit = GetHitDetails(PrimitiveIndex(), hitAttributes);

	//calculate normal in world space
	float3 normal_WS = normalize(mul(hit.normal, (float3x3)ObjectToWorld4x3()));

	//get a unique rng value to offset this ray from other from same pixel
	float2 uv = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();
	float2 rng = Rand2(uv * (payload.recursionDepth + 1) + payload.rayPerPixelIndex + RayTCurrent());

	//get index of refraction depending on which side of object we're on (outside/inside)
	float ior = 1.5f;
	if (HitKind() == HIT_KIND_TRIANGLE_FRONT_FACE) {
		//invert ior for front faces
		ior = 1.0f / ior;
	}
	else {
		//invert normal for back faces
		normal_WS *= -1;
	}

	//random chance for reflection instead of refraction
	float NdotV = dot(-WorldRayDirection(), normal_WS);
	bool reflectFresnel = FresnelSchlick(NdotV, ior) > Rand(rng);

	//test for refraction
	float3 dir;
	if (reflectFresnel || !TryRefract(WorldRayDirection(), normal_WS, ior, dir)) {
			dir = reflect(WorldRayDirection(), normal_WS);

	}

	//lerp between perfect refraction/reflection and random bounce based on roughness squard
	float3 randomBounce = RandomCosineWeightedHemisphere(Rand(rng), Rand(rng.yx), normal_WS);
	//use alpha channel as roughness
	dir = normalize(lerp(dir, randomBounce, saturate(pow(entityColor[InstanceID()].a, 2))));

	RayDesc ray;
	ray.Origin = WorldRayOrigin() + (WorldRayDirection() * RayTCurrent());
	ray.Direction = dir;// reflect(WorldRayDirection(), normal_WS);
	ray.TMin = 0.0001f;
	ray.TMax = 1000.0f;

	//increase recursion depth
	payload.recursionDepth++;

	TraceRay(
		SceneTLAS,
		RAY_FLAG_NONE,
		0xFF, 0, 0, 0, //mask and offsets
		ray,
		payload);
}

//closest hit for emissive objects
[shader("closesthit")]
void ClosestHitEmissive(inout RayPayload payload, BuiltInTriangleIntersectionAttributes hitAttributes) {
	float4 color = entityColor[InstanceID()];
	payload.color = color.rgb * color.a;//use a as intensity
}