#pragma once

#include "CoreMinimal.h"

struct FStaticMeshVertexBuffers;
class FRawStaticIndexBuffer;
class FMultiSizeIndexContainer;

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

	struct FSkinWeightInfo
	{
		uint16 InfluenceBones[4];
		uint8 InfluenceWeights[4];
		friend FArchive& operator<<(FArchive& Ar, FSkinWeightInfo& Info)
		{
			Ar.Serialize(Info.InfluenceBones, sizeof(Info.InfluenceBones));
			Ar.Serialize(Info.InfluenceWeights, sizeof(Info.InfluenceWeights));
			return Ar;
		}
	};
	struct FSkinWeightBuffer
	{
		TArray<FSkinWeightInfo> SkinWeightInfos;
		friend FArchive& operator<<(FArchive& Ar, FSkinWeightBuffer& Buffer)
		{
			// TypeSize and ArrayNum will be serialized first
			// operator<< for FSkinWeightInfo will not be used
			Buffer.SkinWeightInfos.BulkSerialize(Ar);
			return Ar;
		}
	};

	struct FSkelMeshRenderSection
	{
		/** Material (texture) used for this section. */
		int32 MaterialIndex;
		/** The offset of this section's indices in the LOD's index buffer. */
		uint32 BaseIndex;
		/** The number of triangles in this section. */
		uint32 NumTriangles;
		/** The offset into the LOD's vertex buffer of this section's vertices. */
		uint32 BaseVertexIndex;
		/** The number of triangles in this section. */
		uint32 NumVertices;
		/** max # of bones used to skin the vertices in this section */
		int32 MaxBoneInfluences;
		/** This section will cast shadow */
		int32 bCastShadow;
		/** The bones which are used by the vertices of this section. Indices of bones in the USkeletalMesh::RefSkeleton array */
		TArray<uint32> BoneMap;

		FSkelMeshRenderSection()
			: MaterialIndex(0)
			, BaseIndex(0)
			, NumTriangles(0)
			, BaseVertexIndex(0)
			, NumVertices(0)
			, MaxBoneInfluences(4)
			, bCastShadow(1)
		{}

		friend FArchive& operator<<(FArchive& Ar, FSkelMeshRenderSection& Section)
		{
			return Ar << Section.MaterialIndex
				<< Section.BaseIndex
				<< Section.NumTriangles
				<< Section.BaseVertexIndex
				<< Section.NumVertices
				<< Section.MaxBoneInfluences
				<< Section.bCastShadow
				<< Section.BoneMap;
		}
	};
	struct FSkeletalMeshResource
	{
		ns_yoyo::EResourceType Type = EResourceType::SkeletalMesh;
		// asset path as id
		FString Path;
		// sub mesh info
		TArray<FSkelMeshRenderSection> RenderSections;
		// raw vertex data
		FVertexBuffer VertexBuffer;
		// index data
		FIndexBuffer IndexBuffer;
		// skin weight data
		FSkinWeightBuffer SkinWeightBuffer;

		inline friend FArchive& operator<<(FArchive& Ar, FSkeletalMeshResource& Resource)
		{
			// Type must go first
			return Ar << Resource.Type
				<< Resource.Path
				<< Resource.RenderSections
				<< Resource.VertexBuffer
				<< Resource.IndexBuffer
				<< Resource.SkinWeightBuffer;
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

	void ExportVertexBuffer(FVertexBuffer& yyVertexBuffer, FStaticMeshVertexBuffers& ueVertexBuffers);

	void ExportStaticIndexBuffer(FIndexBuffer& yyIndexBuffer, FRawStaticIndexBuffer& ueIndexBuffer);

	void ExportMultiSizeIndexContainer(FIndexBuffer& yyIndexBuffer, FMultiSizeIndexContainer& ueIndexContainer);
}