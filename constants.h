#pragma once

#ifndef CONSTANTS_H
#define CONSTANTS_H

namespace constants
{
    constexpr unsigned short CHUNK_SIZE{ 32 };

	constexpr unsigned int WORLD_BORDER_DISTANCE{ 0 };

	constexpr unsigned int BORDER_DISTANCE_U_B{ (WORLD_BORDER_DISTANCE / CHUNK_SIZE + 1) * CHUNK_SIZE }; //upper bound for world border distance

	constexpr unsigned char skyLightMaxValue{ 15 };

	constexpr int visualTPS{240};

	constexpr float CUBE_FACE_POSITIONS[72] = { 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 0.0f,
												0.0f, 1.0f, 0.0f,
												1.0f, 1.0f, 0.0f,
												
												0.0f, 0.0f, 1.0f,
												1.0f, 0.0f, 1.0f,
												1.0f, 1.0f, 1.0f,
												0.0f, 1.0f, 1.0f,
												
												0.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f,
												0.0f, 1.0f, 1.0f,
												0.0f, 1.0f, 0.0f,
												
												1.0f, 0.0f, 1.0f,
												1.0f, 0.0f, 0.0f,
												1.0f, 1.0f, 0.0f,
												1.0f, 1.0f, 1.0f,
												
												0.0f, 0.0f, 0.0f,
												1.0f, 0.0f, 0.0f,
												1.0f, 0.0f, 1.0f,
												0.0f, 0.0f, 1.0f,
												
												0.0f, 1.0f, 1.0f,
												1.0f, 1.0f, 1.0f,
												1.0f, 1.0f, 0.0f,
												0.0f, 1.0f, 0.0f };

	constexpr float X_FACE_POSITIONS[48] = { 1.0f, 0.0f, 1.0f,
											 0.0f, 0.0f, 0.0f,
											 0.0f, 1.0f, 0.0f,
											 1.0f, 1.0f, 1.0f,
											 
											 0.0f, 0.0f, 0.0f,
											 1.0f, 0.0f, 1.0f,
											 1.0f, 1.0f, 1.0f,
											 0.0f, 1.0f, 0.0f,
											 
											 1.0f, 0.0f, 0.0f,
											 0.0f, 0.0f, 1.0f,
											 0.0f, 1.0f, 1.0f,
											 1.0f, 1.0f, 0.0f,
											 
											 0.0f, 0.0f, 1.0f,
											 1.0f, 0.0f, 0.0f,
											 1.0f, 1.0f, 0.0f,
											 0.0f, 1.0f, 1.0f };

	constexpr float WIREFRAME_CUBE_FACE_POSITIONS[24] = { -0.001f, -0.001f, -0.001f,
													       1.001f, -0.001f, -0.001f,
													       1.001f, -0.001f,  1.001f,
													      -0.001f, -0.001f,  1.001f,
													      
													      -0.001f,  1.001f,  1.001f,
													       1.001f,  1.001f,  1.001f,
													       1.001f,  1.001f, -0.001f,
													      -0.001f,  1.001f, -0.001f };

	constexpr unsigned int CUBE_WIREFRAME_IB[16] = { 0, 1, 2, 3, 0,
													 7, 6, 1, 6, 5, 2, 5, 4, 3, 4, 7 };

	constexpr bool collideable[8] = { false, //air
									  true, //dirt
									  true, //grass
									  true, //stone
									  false, //water
									  true, //oak log
									  true, //oak leaves
									  false //tall grass
	};

	constexpr bool transparent[8] = { true, //air
									  false, //dirt
									  false, //grass
									  false, //stone
									  true, //water
									  false, //oak log
									  true, //oak leaves
									  true //tall grass
	};

	constexpr bool castsShadows[8] = { false, //air
									   true, //dirt
									   true, //grass
									   true, //stone
									   false, //water
									   true, //oak log
									   false, //oak leaves
									   false //tall grass
	};

	constexpr bool dimsLight[8] = { false, //air
									false, //dirt
									false, //grass
									false, //stone
									true, //water
									false, //oak log
									true, //oak leaves
									false //tall grass
	};

	constexpr bool cubeMesh[8] = { true, //air
							   true, //dirt
							   true, //grass
							   true, //stone
							   true, //water
							   true, //oak log
							   true, //oak leaves
							   false //tall grass
	};

	constexpr bool xMesh[8] = { false, //air
							    false, //dirt
							    false, //grass
							    false, //stone
							    false, //water
							    false, //oak log
							    false, //oak leaves
							    true //tall grass
	};

	constexpr float shadowReceiveAmount[8] = { 0.65f, //air
											   0.65f, //dirt
											   0.65f, //grass
											   0.65f, //stone
											   0.65f, //water
											   0.65f, //oak log
											   0.85f, //oak leaves
											   0.65f //tall grass
	};
}
#endif