#pragma once

#include "JunJsonAsset.generated.h"

USTRUCT()
struct FJsonMesh
{
	GENERATED_BODY()

	// 暂时只导出坐标
	UPROPERTY()
	TArray<FVector> Positions;

	// 暂不考虑uint16的优化
	UPROPERTY()
	TArray<uint32> Indices;
};

// construct front, right, up from yaw, pitch, roll
// see FRotationTranslationMatrix
USTRUCT()
struct FJsonCamera
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position;

	UPROPERTY()
	float Yaw;

	UPROPERTY()
	float Pitch;

	UPROPERTY()
	float Roll;

	UPROPERTY()
	int32 Fov;

	UPROPERTY()
	float AspectRatio;
};