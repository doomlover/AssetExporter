// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetExporterBPLibrary.h"
#include "AssetExporter.h"
#include "Animation/AnimTypes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Classes/Animation/SkeletalMeshActor.h"
#include "Engine/Classes/Animation/AnimSequence.h"
#include "Engine/Classes/Animation/Skeleton.h"
#include "Engine/Classes/GameFramework/Character.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
//#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "StaticMeshResources.h"
#include "SingleAnimationPlayData.h"
#include "ReferenceSkeleton.h"

#include "ExportTypes.h"

template<typename T>
bool SerializeToFile(T& Obj, const FString& Path)
{
	FString SavePath = Path + Obj.Path;
	TArray<uint8> ByteData;
	FMemoryWriter BytesWriter(ByteData);
	BytesWriter << Obj;
	return FFileHelper::SaveArrayToFile(ByteData, *SavePath);
}
#if 1
template<ns_yoyo::EResourceType ResourceType>
FString GetAssetPath(UObject* Asset)
{
	auto package_path = Asset->GetPackage()->GetPathName();
	package_path.ReplaceInline(TEXT("/Game"), TEXT(""));
	switch (ResourceType)
	{
	case ns_yoyo::EResourceType::Level:
		package_path += TEXT(".scene");
		break;
	case ns_yoyo::EResourceType::StaticMesh:
		package_path += TEXT(".mesh");
		break;
	case ns_yoyo::EResourceType::SkeletalMesh:
		package_path += TEXT(".skelmesh");
		break;
	case ns_yoyo::EResourceType::AnimSequence:
		package_path += TEXT(".anim");
		break;
	case ns_yoyo::EResourceType::Skeleton:
		package_path += TEXT(".skel");
		break;
	case ns_yoyo::EResourceType::Max:
	default:
		check(false);
		break;
	}
	return package_path;
}
#else
FString GetAssetPath(UObject* Asset)
{
	auto package_path = Asset->GetPackage()->GetPathName();
	package_path.ReplaceInline(TEXT("/Game"), TEXT(""));
	return package_path;
}
#endif

UAssetExporterBPLibrary::UAssetExporterBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

void UAssetExporterBPLibrary::ExportCamera(ACameraActor* Camera,
	ns_yoyo::FLevelSceneInfo& LevelSceneInfo)
{
	check(Camera && Camera->IsA(ACameraActor::StaticClass()));
#if 1
	UCameraComponent* CameraComponent = Camera->GetCameraComponent();
	FVector Location = CameraComponent->GetComponentLocation();
	FVector Up = CameraComponent->GetUpVector();
	FVector Right = CameraComponent->GetRightVector();
	FVector Forward = CameraComponent->GetForwardVector();
	float Fov = CameraComponent->FieldOfView;
	float AspectRatio = CameraComponent->AspectRatio;

	LevelSceneInfo.Camera.AspectRatio = AspectRatio;
	LevelSceneInfo.Camera.Fov = Fov;
	LevelSceneInfo.Camera.Forward = Forward;
	LevelSceneInfo.Camera.Location = Location;
	LevelSceneInfo.Camera.Right = Right;
	LevelSceneInfo.Camera.Up = Up;
#else
	TSharedPtr<FJsonCamera> JsonCamea = MakeShareable(new FJsonCamera);

	UCameraComponent* CameraComponent = Camera->GetCameraComponent();
	JsonCamea->Position = CameraComponent->GetComponentLocation();
	JsonCamea->Fov = CameraComponent->FieldOfView;
	JsonCamea->AspectRatio = CameraComponent->AspectRatio;
	FRotator Rotator = CameraComponent->GetComponentRotation();
	JsonCamea->Yaw = Rotator.Yaw;
	JsonCamea->Pitch = Rotator.Pitch;
	JsonCamea->Roll = Rotator.Roll;

	/* 
	* debug
	* 虚幻以Z轴为Up，DirectX/OpenGL以Y轴为Up
	* 以下设FVector(0, 1, 0)为up
	*/
	/*FVector Front0 = CameraComponent->GetForwardVector();

	FVector Front1;
	Front1.X = FMath::Cos(FMath::DegreesToRadians(JsonCamea->Yaw)) * FMath::Cos(FMath::DegreesToRadians(JsonCamea->Pitch));
	Front1.Y = FMath::Sin(FMath::DegreesToRadians(JsonCamea->Pitch));
	Front1.Z = FMath::Sin(FMath::DegreesToRadians(JsonCamea->Yaw)) * FMath::Cos(FMath::DegreesToRadians(JsonCamea->Pitch));
	Front1.Normalize();

	FVector Up0 = CameraComponent->GetUpVector();
	FVector Right0 = CameraComponent->GetRightVector();

	FVector Up1, Right1;
	Right1 = FVector::CrossProduct(Front1, FVector(0, 1, 0));
	Right1.Normalize();

	Up1 = FVector::CrossProduct(Right1, Front1);
	Up1.Normalize();

	UE_LOG(LogTemp, Log, TEXT("ue f:%s, r:%s, u:%s"), *Front0.ToString(), *Right0.ToString(), *Up0.ToString());
	UE_LOG(LogTemp, Log, TEXT("mine f:%s, r:%s, u:%s"), *Front1.ToString(), *Right1.ToString(), *Up1.ToString());*/

	// export to json and save
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(*JsonCamea, JsonStr);
	FString lPath = Path;
	if (lPath.IsEmpty())
	{
		lPath = FPackageName::GetLongPackagePath(Camera->GetPackage()->GetPathName()) + TEXT("/") + Camera->GetActorLabel();
		lPath = FPackageName::LongPackageNameToFilename(lPath);
	}
	FFileHelper::SaveStringToFile(JsonStr, *lPath);
#endif
}

void UAssetExporterBPLibrary::ExportDirectionalLight(ADirectionalLight* UELight,
	ns_yoyo::FLevelSceneInfo& yyLevelSceneInfo)
{
	check(UELight && UELight->IsA(ADirectionalLight::StaticClass()));
	UDirectionalLightComponent* LightComponent = UELight->GetComponent();
	auto& yyLight = yyLevelSceneInfo.DirectionalLight;
	FLinearColor Color = LightComponent->GetLightColor();
	yyLight.Color = {Color.R, Color.G, Color.B};
	yyLight.Direction = LightComponent->GetComponentRotation().Vector();
	yyLight.Intensity = LightComponent->Intensity;
}

void UAssetExporterBPLibrary::ExportSkeletalMesh(USkeletalMesh* SkelMesh, const FString& Path)
{
	ns_yoyo::FSkeletalMeshResource yySkeletalMeshResource;

	check(SkelMesh);
	FSkeletalMeshRenderData* RenderData = SkelMesh->GetResourceForRendering();
	check(RenderData && RenderData->LODRenderData.Num() > 0);
	FSkeletalMeshLODRenderData& LOD0 = RenderData->LODRenderData[0];

	// fill the path
	yySkeletalMeshResource.Path = GetAssetPath<ns_yoyo::EResourceType::SkeletalMesh>(SkelMesh);
	yySkeletalMeshResource.SkelAssetPath = GetAssetPath<ns_yoyo::EResourceType::Skeleton>(SkelMesh->Skeleton);

	// fill the sections
	int32 NumTriangles = 0;
	for (FSkelMeshRenderSection& ueSection : LOD0.RenderSections)
	{
		ns_yoyo::FSkelMeshRenderSection yySkelMeshRenderSection;
		yySkelMeshRenderSection.BaseIndex = ueSection.BaseIndex;
		yySkelMeshRenderSection.BaseVertexIndex = ueSection.BaseVertexIndex;
		yySkelMeshRenderSection.bCastShadow = ueSection.bCastShadow;
		yySkelMeshRenderSection.MaterialIndex = ueSection.MaterialIndex;
		yySkelMeshRenderSection.MaxBoneInfluences = ueSection.MaxBoneInfluences;
		yySkelMeshRenderSection.NumTriangles = ueSection.NumTriangles;
		yySkelMeshRenderSection.NumVertices = ueSection.NumVertices;
		yySkelMeshRenderSection.BoneMap.Reserve(ueSection.BoneMap.Num());
		for (auto& BoneIndex : ueSection.BoneMap)
		{
			yySkelMeshRenderSection.BoneMap.Add(BoneIndex);
		}
		yySkeletalMeshResource.RenderSections.Add(yySkelMeshRenderSection);
		NumTriangles += ueSection.NumTriangles;
	}

	// vertex buffer
	ns_yoyo::ExportVertexBuffer(yySkeletalMeshResource.VertexBuffer, LOD0.StaticVertexBuffers);

	// index buffer
	ns_yoyo::ExportMultiSizeIndexContainer(yySkeletalMeshResource.IndexBuffer, LOD0.MultiSizeIndexContainer);
	check(yySkeletalMeshResource.IndexBuffer.NumIndices == NumTriangles * 3);

	// skin weight buffer
	FSkinWeightVertexBuffer* WeightVertexBuffer = LOD0.GetSkinWeightVertexBuffer();
	TArray<FSkinWeightInfo> SkinWeightInfos;
	WeightVertexBuffer->GetSkinWeights(SkinWeightInfos);
	for (auto& WeightInfo : SkinWeightInfos)
	{
		ns_yoyo::FSkinWeightInfo yyInfo;
		FMemory::Memcpy(yyInfo.InfluenceBones, WeightInfo.InfluenceBones, sizeof(FBoneIndexType) * 4);
		FMemory::Memcpy(yyInfo.InfluenceWeights, WeightInfo.InfluenceWeights, sizeof(uint8) * 4);
		yySkeletalMeshResource.SkinWeightBuffer.SkinWeightInfos.Emplace(yyInfo);
	}

	// serialize to file
	bool bOk = SerializeToFile(yySkeletalMeshResource, Path);
	check(bOk);
}

void UAssetExporterBPLibrary::ExportAnimSequence(UAnimSequence* AnimSequence, const FString& Path)
{
	check(AnimSequence);
	const TArray<FRawAnimSequenceTrack>& BoneTracks = AnimSequence->GetRawAnimationData();
	int32 NumRawFrames = AnimSequence->GetRawNumberOfFrames();
	int32 NumFrames = AnimSequence->GetNumberOfFrames();
	//const TArray<FTrackToSkeletonMap>& TrackBoneIndices = AnimSequence->GetRawTrackToSkeletonMapTable();

	ns_yoyo::FAnimSequenceResource yyAnimSequence;
	yyAnimSequence.Path = GetAssetPath<ns_yoyo::EResourceType::AnimSequence>(AnimSequence);
	yyAnimSequence.NumFrames = NumRawFrames;
	yyAnimSequence.RawAnimationData.AddZeroed(BoneTracks.Num());
	for (int32 i = 0; i < BoneTracks.Num(); ++i)
	{
		yyAnimSequence.RawAnimationData[i].PosKeys = BoneTracks[i].PosKeys;
		yyAnimSequence.RawAnimationData[i].RotKeys = BoneTracks[i].RotKeys;
		yyAnimSequence.RawAnimationData[i].ScaleKeys = BoneTracks[i].ScaleKeys;
	}
	USkeleton* Skeleton = AnimSequence->GetSkeleton();
	check(Skeleton);
	yyAnimSequence.SkelAssetPath = GetAssetPath<ns_yoyo::EResourceType::Skeleton>(Skeleton);

	bool bOk = SerializeToFile(yyAnimSequence, Path);
	check(bOk);
}

void UAssetExporterBPLibrary::ExportSkeleton(USkeleton* Skeleton, const FString& Path)
{
	check(Skeleton);
	const FReferenceSkeleton& ReferenceSkel = Skeleton->GetReferenceSkeleton();
	const TArray<FMeshBoneInfo>& BoneInfo = ReferenceSkel.GetRawRefBoneInfo();
	const TArray<FTransform>& BonePose = ReferenceSkel.GetRawRefBonePose();

	ns_yoyo::FSkeleton yySkeleton;
	yySkeleton.Path = GetAssetPath<ns_yoyo::EResourceType::Skeleton>(Skeleton);
	yySkeleton.BoneInfos.AddZeroed(BoneInfo.Num());
	for (int32 i = 0; i < BoneInfo.Num(); ++i)
	{
		yySkeleton.BoneInfos[i].Name = BoneInfo[i].ExportName;
		yySkeleton.BoneInfos[i].ParentIndex = BoneInfo[i].ParentIndex;
	}
	yySkeleton.BonePoses.AddZeroed(BonePose.Num());
	for (int32 i = 0; i < BonePose.Num(); ++i)
	{
		//yySkeleton.BonePoses[i].Rot = ns_yoyo::Quat2Vec4(BonePose[i].Rotator().Quaternion());
		yySkeleton.BonePoses[i].Rot = BonePose[i].Rotator().Quaternion();
		yySkeleton.BonePoses[i].Trans = BonePose[i].GetLocation();
		yySkeleton.BonePoses[i].Scale = BonePose[i].GetScale3D();
	}

	bool bOk = SerializeToFile(yySkeleton, Path);
	check(bOk);
}

void UAssetExporterBPLibrary::ExportStaticMesh(UStaticMesh* Mesh, const FString& Path)
{
	// check and get the lod0 resource
	check(Mesh && Mesh->RenderData && Mesh->RenderData->LODResources.Num() > 0);
	FStaticMeshLODResources& LODResource = Mesh->RenderData->LODResources[0];
	const int32 NumVerts = LODResource.GetNumVertices();
	const int32 NumTris = LODResource.GetNumTriangles();

	ns_yoyo::FStaticMeshResource yyMeshResource;

	// build resource path
	yyMeshResource.Path = GetAssetPath<ns_yoyo::EResourceType::StaticMesh>(Mesh);

	// sections
	for (auto& ueSection : LODResource.Sections)
	{
		ns_yoyo::FStaticMeshSection yySection;
		yySection.FirstIndex = ueSection.FirstIndex;
		yySection.MaterialIndex = ueSection.MaterialIndex;
		yySection.MaxVertexIndex = ueSection.MaxVertexIndex;
		yySection.MinVertexIndex = ueSection.MinVertexIndex;
		yySection.NumTriangles = ueSection.NumTriangles;
		yySection.bCastShadow = ueSection.bCastShadow;
		yyMeshResource.Sections.Add(yySection);
	}
	
	// vertex buffer
	ns_yoyo::ExportVertexBuffer(yyMeshResource.VertexBuffer, LODResource.VertexBuffers);

	// index buffer
	ns_yoyo::ExportStaticIndexBuffer(yyMeshResource.IndexBuffer, LODResource.IndexBuffer);
	check(yyMeshResource.IndexBuffer.NumIndices == NumTris * 3);

	// serialize to file
	bool bOk = SerializeToFile(yyMeshResource, Path);
	check(bOk);
}

void UAssetExporterBPLibrary::ExportAsset(UObject* Asset, const FString& Path)
{
	if (Asset->IsA(UWorld::StaticClass()))
	{
		return ExportMap(Cast<UWorld>(Asset), Path);
	}
	if (Asset->IsA(UStaticMesh::StaticClass()))
	{
		return ExportStaticMesh(Cast<UStaticMesh>(Asset), Path);
	}
	if (Asset->IsA(USkeletalMesh::StaticClass()))
	{
		return ExportSkeletalMesh(Cast<USkeletalMesh>(Asset), Path);
	}
	if (Asset->IsA(UAnimSequence::StaticClass()))
	{
		return ExportAnimSequence(Cast<UAnimSequence>(Asset), Path);
	}
	if (Asset->IsA(USkeleton::StaticClass()))
	{
		return ExportSkeleton(Cast<USkeleton>(Asset), Path);
	}
}

void UAssetExporterBPLibrary::ExportMap(UWorld* World, const FString& Path)
{
	UE_LOG(LogTemp, Log, TEXT("RootPath = "), *Path);

	ns_yoyo::FLevelSceneInfo yySceneInfo;

	TSet<UStaticMesh*> ExportedStaticMeshes;
	TSet<USkeletalMesh*> ExportedSkelMeshes;
	TSet<UAnimSequence*> ExportedAnimSequences;
	TSet<USkeleton*> ExportedSkeletons;

	ULevel* Level = World->PersistentLevel;
	for (AActor* Actor : Level->Actors)
	{
		// static mesh
		if (Actor->IsA(AStaticMeshActor::StaticClass()))
		{
			TArray<UStaticMeshComponent*> StaticMeshComponents;
			Actor->GetComponents(StaticMeshComponents);
			// export every static mesh component
			for (auto Component : StaticMeshComponents)
			{
				UStaticMesh* StaticMesh =
					Cast<UStaticMeshComponent>(Component)->GetStaticMesh();
				if (StaticMesh)
				{
					// cache the mesh for exporting
					ExportedStaticMeshes.Add(StaticMesh);

					// build static mesh scene info
					ns_yoyo::FStaticMeshSceneInfo yyStaticMeshSceneInfo;
					yyStaticMeshSceneInfo.ResourcePath = 
						GetAssetPath<ns_yoyo::EResourceType::StaticMesh>(StaticMesh);
					yyStaticMeshSceneInfo.Location =
						Cast<UPrimitiveComponent>(Component)->GetComponentLocation();
					yyStaticMeshSceneInfo.Rotation =
						Cast<UPrimitiveComponent>(Component)->GetComponentRotation().Quaternion();
					yyStaticMeshSceneInfo.Scale =
						Cast<UPrimitiveComponent>(Component)->GetComponentScale();
					yySceneInfo.StaticMesheSceneInfos.Emplace(yyStaticMeshSceneInfo);
				}
			}
			continue;
		}
		if (Actor->IsA(ASkeletalMeshActor::StaticClass()) ||
			Actor->IsA(ACharacter::StaticClass()))
		{
			TArray<USkeletalMeshComponent*> SkelMeshComponents;
			Actor->GetComponents(SkelMeshComponents);
			for (auto Component : SkelMeshComponents)
			{
				USkeletalMesh* SkelMesh = Component->SkeletalMesh;
				if (SkelMesh)
				{
					ExportedSkelMeshes.Add(SkelMesh);
					// build skeletal mesh scene info
					ns_yoyo::FSkeletalMeshSceneInfo yySkelMeshSceneInfo;
					yySkelMeshSceneInfo.ResourcePath = GetAssetPath<ns_yoyo::EResourceType::SkeletalMesh>(SkelMesh);
					yySkelMeshSceneInfo.Transform.Rot = Component->GetComponentQuat();
					yySkelMeshSceneInfo.Transform.Trans = Component->GetComponentLocation();
					yySkelMeshSceneInfo.Transform.Scale = Component->GetComponentScale();
					yySceneInfo.SkelMeshSceneInfos.Emplace(yySkelMeshSceneInfo);
				}
				if (EAnimationMode::AnimationSingleNode == Component->GetAnimationMode())
				{
					UAnimSequence* AnimSequence = Cast<UAnimSequence>(Component->AnimationData.AnimToPlay);
					if (AnimSequence)
					{
						ExportedAnimSequences.Add(AnimSequence);
						USkeleton* Skeleton = AnimSequence->GetSkeleton();
						check(Skeleton);
						ExportedSkeletons.Add(Skeleton);
					}
				}
			}
			continue;
		}
		if (Actor->IsA(ACameraActor::StaticClass()))
		{
			ExportCamera(Cast<ACameraActor>(Actor), yySceneInfo);
			continue;
		}
		if (Actor->IsA(ADirectionalLight::StaticClass()))
		{
			ExportDirectionalLight(Cast<ADirectionalLight>(Actor), yySceneInfo);
			continue;
		}
	}

	// export static meshes
	for (UStaticMesh* StaticMesh : ExportedStaticMeshes)
	{
		// export static mesh asset
		ExportStaticMesh(StaticMesh, Path);
	}

	// export skeletal meshes
	for (USkeletalMesh* SkelMesh : ExportedSkelMeshes)
	{
		ExportSkeletalMesh(SkelMesh, Path);
	}

	// export animation sequences
	for (UAnimSequence* AnimSeq : ExportedAnimSequences)
	{
		ExportAnimSequence(AnimSeq, Path);
	}

	// export skeletons
	for (USkeleton* Skeleton : ExportedSkeletons)
	{
		ExportSkeleton(Skeleton, Path);
	}

	// write to file
	ns_yoyo::FLevelResource yyLevelResource;
	yyLevelResource.Path = GetAssetPath<ns_yoyo::EResourceType::Level>(Level);
	yyLevelResource.SceneInfo = yySceneInfo;

#if 1
	bool bOk = SerializeToFile(yyLevelResource, Path);
	check(bOk);
#else
	TArray<uint8> ByteData;
	FMemoryWriter BytesWriter(ByteData);
	BytesWriter << yyLevelResource;
	bool bOk = FFileHelper::SaveArrayToFile(ByteData, *SavePath);
	check(bOk);
#endif // 1
}

/*
struct FJunMesh
{
	int32 NumVerts;
	int32 NumIndices;
	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<uint32> Indices;
};
void UAssetExporterBPLibrary::ExportStaticMeshJson(UStaticMesh* Mesh, const FString& Path)
{
	///MeshDescription带有冗余信息，UE4通过MeshDescription重建有BUG，只能得到带冗余的重建结果.
	///所以还是通过LodResource重建.
	///
#if 1
	check(Mesh && Mesh->RenderData && Mesh->RenderData->LODResources.Num() > 0);
	// 只处理LOD0并认为只有一个材质球
	FStaticMeshLODResources& LODResource = Mesh->RenderData->LODResources[0];
	int32 NumVerts = LODResource.GetNumVertices();
	TSharedPtr<FJsonMesh> JunMesh = MakeShareable(new FJsonMesh);
	JunMesh->Positions.SetNum(NumVerts);
	FMemory::Memcpy(JunMesh->Positions.GetData(), LODResource.VertexBuffers.PositionVertexBuffer.GetVertexData(), NumVerts * sizeof(FVector));

	int32 NumTris = LODResource.GetNumTriangles();
	FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
	IndexBuffer.GetCopy(JunMesh->Indices);
	check(JunMesh->Indices.Num() == NumTris * 3);
#else
	// get mesh description
	FMeshDescription* MeshDescriptionPtr = Mesh->GetMeshDescription(0);
	check(MeshDescriptionPtr);
	FMeshDescription& MeshDescription = *MeshDescriptionPtr;
	FStaticMeshConstAttributes MeshDescriptionAttributes(MeshDescription);

	TSharedPtr<FJsonMesh> JunMesh = MakeShareable(new FJsonMesh);

	// get vertices
	int32 NumVertexInstances = MeshDescription.VertexInstances().GetArraySize();
	JunMesh->Positions.SetNum(NumVertexInstances);

	TVertexAttributesConstRef<FVector> VertexPositions = MeshDescriptionAttributes.GetVertexPositions();
	for (FVertexInstanceID VertexInstanceID : MeshDescription.VertexInstances().GetElementIDs())
	{
		JunMesh->Positions.Add(VertexPositions[MeshDescription.GetVertexInstanceVertex(VertexInstanceID)]);
	}

	// get indices
	int32 NumTriangles = MeshDescription.Triangles().Num();
	JunMesh->Indices.SetNumZeroed(NumTriangles * 3);

	for (FPolygonGroupID PolygonGroupID : MeshDescription.PolygonGroups().GetElementIDs())
	{
		if (MeshDescription.GetNumPolygonGroupPolygons(PolygonGroupID) == 0)
		{
			continue;
		}
		for (FPolygonID PolygonID : MeshDescription.GetPolygonGroupPolygons(PolygonGroupID))
		{
			for (FTriangleID TriangleID : MeshDescription.GetPolygonTriangleIDs(PolygonID))
			{
				for (FVertexInstanceID TriangleVertexInstanceIDs : MeshDescription.GetTriangleVertexInstances(TriangleID))
				{
					uint32 VertexIndex = static_cast<uint32>(TriangleVertexInstanceIDs.GetValue());
					JunMesh->Indices.Add(VertexIndex);
				}
			}
		}
	}
#endif

	// export to json and save
	FString JsonStr;
	FJsonObjectConverter::UStructToJsonObjectString(*JunMesh, JsonStr);
	FString lPath = Path;
	if (lPath.IsEmpty())
	{
		lPath = FPackageName::LongPackageNameToFilename(Mesh->GetPackage()->GetPathName());
	}
	FFileHelper::SaveStringToFile(JsonStr, *lPath);
}*/