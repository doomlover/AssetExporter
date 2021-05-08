// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AssetExporterBPLibrary.generated.h"

/* 
*	Function library class.
*	Each function in it is expected to be static and represents blueprint node that can be called in any blueprint.
*
*	When declaring function you can define metadata for the node. Key function specifiers will be BlueprintPure and BlueprintCallable.
*	BlueprintPure - means the function does not affect the owning object in any way and thus creates a node without Exec pins.
*	BlueprintCallable - makes a function which can be executed in Blueprints - Thus it has Exec pins.
*	DisplayName - full name of the node, shown when you mouse over the node and in the blueprint drop down menu.
*				Its lets you name the node using characters not allowed in C++ function names.
*	CompactNodeTitle - the word(s) that appear on the node.
*	Keywords -	the list of keywords that helps you to find node when you search for it using Blueprint drop-down menu. 
*				Good example is "Print String" node which you can find also by using keyword "log".
*	Category -	the category your node will be under in the Blueprint drop-down menu.
*
*	For more info on custom blueprint nodes visit documentation:
*	https://wiki.unrealengine.com/Custom_Blueprint_Node_Creation
*/

class UStaticMesh;
class ACameraActor;
class ADirectionalLight;

namespace ns_yoyo
{
	struct FLevelSceneInfo;
}

UCLASS()
class ASSETEXPORTER_API UAssetExporterBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "AssetExporter sample test testing"), Category = "AssetExporterTesting")
	static float AssetExporterSampleFunction(float Param);

	UFUNCTION(BlueprintCallable)
	static void ExportStaticMeshJson(UStaticMesh* Mesh, const FString& Path);

	UFUNCTION(BlueprintCallable)
	static void ExportStaticMesh(UStaticMesh* Mesh, const FString& Path);

	UFUNCTION(BlueprintCallable)
	static void ExportAsset(UObject* Asset, const FString& Path);

	static void ExportMap(UWorld* World, const FString& Path);

	static void ExportCamera(ACameraActor* Camera,
		ns_yoyo::FLevelSceneInfo& LevelSceneInfo);

	static void ExportDirectionalLight(ADirectionalLight* DirectionalLight,
		ns_yoyo::FLevelSceneInfo& LevelSceneInfo);
};

namespace ns_yoyo
{
	enum class EResourceType : uint8
	{
		Level,
		StaticMesh,
		SkeletalMesh,
		Max
	};

	class FVertexBuffer
	{
	public:
		uint32 Stride;
		uint32 NumVertices;
		TArray<float> RawData;
	
		friend FArchive& operator<<(FArchive& Ar, FVertexBuffer& Buffer)
		{
			return Ar << Buffer.Stride
				<< Buffer.NumVertices
				<< Buffer.RawData;
		}
	};

	class FIndexBuffer
	{
	public:
		uint32 NumIndices;
		TArray<uint32> BufferData;
	
		friend FArchive& operator<<(FArchive& Ar, FIndexBuffer& Buffer)
		{
			return Ar << Buffer.NumIndices
				<< Buffer.BufferData;
		}
	};

	struct FStaticMeshSection
	{
		int32 MaterialIndex;
		uint32 FirstIndex;
		uint32 NumTriangles;
		uint32 MinVertexIndex;
		uint32 MaxVertexIndex;
		bool bCastShadow;

		FStaticMeshSection()
			: MaterialIndex(0)
			, FirstIndex(0)
			, NumTriangles(0)
			, MinVertexIndex(0)
			, MaxVertexIndex(0)
			, bCastShadow(true)
		{}

		friend FArchive& operator<<(FArchive& Ar, FStaticMeshSection& Section)
		{
			return Ar << Section.MaterialIndex
				<< Section.FirstIndex
				<< Section.NumTriangles
				<< Section.MinVertexIndex
				<< Section.MaxVertexIndex
				<< Section.bCastShadow;
		}
	};

	struct FStaticMeshResource
	{
		ns_yoyo::EResourceType Type = EResourceType::StaticMesh;
		// asset path as id
		FString Path;
		// sub mesh info
		TArray<FStaticMeshSection> Sections;
		// raw vertex data
		FVertexBuffer VertexBuffer;
		// index data
		FIndexBuffer IndexBuffer;

		// material

		inline friend FArchive& operator<<(FArchive& Ar, FStaticMeshResource& Resource)
		{
			// Type must go first
			return Ar << Resource.Type
				<< Resource.Path
				<< Resource.Sections
				<< Resource.VertexBuffer
				<< Resource.IndexBuffer;
		}
	};

	struct FStaticMeshSceneInfo
	{
		// path relative to the Content folder
		FString ResourcePath;
		// world transformation
		FVector Location;
		FQuat Rotation;
		FVector Scale;
	};

	inline FArchive& operator<<(FArchive& Ar,
		FStaticMeshSceneInfo& StaticMeshSceneInfo)
	{
		return Ar << StaticMeshSceneInfo.ResourcePath
		<< StaticMeshSceneInfo.Location
		<< StaticMeshSceneInfo.Rotation
		<< StaticMeshSceneInfo.Scale;
	}

	struct FDirectionalLightSceneInfo
	{
		FVector Direction;
		FVector Color;
		float Intensity;

		friend FArchive& operator<<(FArchive& Ar,
			FDirectionalLightSceneInfo& Light)
		{
			return Ar << Light.Direction
				<< Light.Color
				<< Light.Intensity;
		}
	};

	struct FCameraSceneInfo
	{
		FVector Location;
		FVector Up;
		FVector Right;
		FVector Forward;
		float Fov;
		float AspectRatio;

		friend FArchive& operator<<(FArchive& Ar, FCameraSceneInfo& CameraSceneInfo)
		{
			return Ar << CameraSceneInfo.Location
				<< CameraSceneInfo.Up
				<< CameraSceneInfo.Right
				<< CameraSceneInfo.Forward
				<< CameraSceneInfo.Fov
				<< CameraSceneInfo.AspectRatio;
		}
	};

	struct FLevelSceneInfo
	{
		FCameraSceneInfo Camera;
		FDirectionalLightSceneInfo DirectionalLight;
		TArray<FStaticMeshSceneInfo> StaticMesheSceneInfos;

		friend FArchive& operator<<(FArchive& Ar, FLevelSceneInfo& SceneInfo)
		{
			Ar << SceneInfo.Camera;
			Ar << SceneInfo.DirectionalLight;
			Ar << SceneInfo.StaticMesheSceneInfos;
			return Ar;
		}
	};

	struct FLevelResource
	{
		EResourceType Type = EResourceType::Level;
		FString Path;
		FLevelSceneInfo SceneInfo;
		friend FArchive& operator<<(FArchive& Ar, FLevelResource& LevelResource)
		{
			// Type must go first
			return Ar << LevelResource.Type
				<< LevelResource.Path
				<< LevelResource.SceneInfo;
		}
	};
}
