// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetExporterBPLibrary.h"
#include "AssetExporter.h"
#include "JsonObjectConverter.h"
#include "JunJsonAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "Components/PrimitiveComponent.h"
#include "Components/DirectionalLightComponent.h"
#if 0
#include "StaticMeshAttributes.h"
#include "MeshElementArray.h"
#endif

FString GetAssetPath(UObject* Asset)
{
	auto package_path = Asset->GetPackage()->GetPathName();
	package_path.ReplaceInline(TEXT("/Game"), TEXT(""));
	return package_path;
}

UAssetExporterBPLibrary::UAssetExporterBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float UAssetExporterBPLibrary::AssetExporterSampleFunction(float Param)
{
	return -1;
}

void UAssetExporterBPLibrary::ExportStaticMeshJson(UStaticMesh* Mesh, const FString& Path)
{
	/* MeshDescription带有冗余信息，UE4通过MeshDescription重建有BUG，只能得到带冗余的重建结果.
	*  所以还是通过LodResource重建.
	*/
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

struct FJunMesh
{
	int32 NumVerts;
	int32 NumIndices;
	TArray<FVector> Positions;
	TArray<FVector> Normals;
	TArray<uint32> Indices;
};

/*
Export mesh to a custom formated binary file. See FJunMesh.
Just lod0 will be exported and one material is supported.
*/
void UAssetExporterBPLibrary::ExportStaticMesh(UStaticMesh* Mesh, const FString& Path)
{
	// check and get the lod0 resource
	check(Mesh && Mesh->RenderData && Mesh->RenderData->LODResources.Num() > 0);
	FStaticMeshLODResources& LODResource = Mesh->RenderData->LODResources[0];
	const int32 NumVerts = LODResource.GetNumVertices();
	const int32 NumTris = LODResource.GetNumTriangles();

	ns_yoyo::FStaticMeshResource yyMeshResource;
	// build resource path
	/*auto mesh_path = Mesh->GetPathName();
	auto package_path = Mesh->GetPackage()->GetPathName();
	auto package_file = FPackageName::LongPackageNameToFilename(Mesh->GetPackage()->GetPathName());
	package_path.ReplaceInline(TEXT("/Game"), TEXT(""));*/
	yyMeshResource.Path = GetAssetPath(Mesh);

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
	yyMeshResource.VertexBuffer.NumVertices = NumVerts;
	yyMeshResource.VertexBuffer.Stride = 32; // bytes
	auto& FloatRawData = yyMeshResource.VertexBuffer.RawData;
	for (int32 i = 0; i < NumVerts; ++i)
	{
		// position
		auto Position = LODResource.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
		FloatRawData.Append({ Position.X, Position.Y, Position.Z });
		// normal
		auto Normal = LODResource.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
		FloatRawData.Append({ Normal.X, Normal.Y, Normal.Z });
		// uv
		auto UV = LODResource.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
		FloatRawData.Append({UV.X, UV.Y});
	}

	// index buffer
	yyMeshResource.IndexBuffer.NumIndices = NumTris * 3;
	LODResource.IndexBuffer.GetCopy(yyMeshResource.IndexBuffer.BufferData);

	TArray<uint8> ByteData;
	FMemoryWriter BytesWriter(ByteData);
	BytesWriter << yyMeshResource;
	auto save_path = Path + yyMeshResource.Path;
	bool bOk = FFileHelper::SaveArrayToFile(ByteData, *save_path);
#if 0
	TSharedPtr<FJunMesh> JunMesh = MakeShareable(new FJunMesh);
	JunMesh->NumVerts = NumVerts;
	// positions
	JunMesh->Positions.SetNum(NumVerts);
	check(LODResource.VertexBuffers.PositionVertexBuffer.GetNumVertices() == NumVerts);
	check(LODResource.VertexBuffers.PositionVertexBuffer.GetStride());
	FMemory::Memcpy(JunMesh->Positions.GetData(), LODResource.VertexBuffers.PositionVertexBuffer.GetVertexData(), NumVerts * sizeof(FVector));

	// normals
	JunMesh->Normals.SetNum(NumVerts);
	FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODResource.VertexBuffers.StaticMeshVertexBuffer;
	for (int32 i = 0; i < NumVerts; ++i)
	{
		JunMesh->Normals[i] = (FVector)StaticMeshVertexBuffer.VertexTangentZ(i);
	}

	// indices
	JunMesh->NumIndices = NumTris * 3;
	FRawStaticIndexBuffer& IndexBuffer = LODResource.IndexBuffer;
	IndexBuffer.GetCopy(JunMesh->Indices);
	check(JunMesh->Indices.Num() == JunMesh->NumIndices);

	// export to file
	FString lPath = Path;
	if (lPath.IsEmpty())
	{
		lPath = FPackageName::LongPackageNameToFilename(Mesh->GetPackage()->GetPathName());
	}
	TArray<uint8> JunMeshBytes;
	JunMeshBytes.SetNum(sizeof(int32) * 2
		+(JunMesh->Positions.Num() + JunMesh->Normals.Num()) * sizeof(FVector)
		+JunMesh->Indices.Num() * sizeof(uint32));
	FMemory::Memcpy(JunMeshBytes.GetData(), &JunMesh->NumVerts, sizeof(int32));
	FMemory::Memcpy(JunMeshBytes.GetData() + sizeof(int32), &JunMesh->NumIndices, sizeof(int32));
	FMemory::Memcpy(JunMeshBytes.GetData() + sizeof(int32) * 2, JunMesh->Positions.GetData(), JunMesh->Positions.Num() * sizeof(FVector));
	FMemory::Memcpy(JunMeshBytes.GetData() + sizeof(int32) * 2 + JunMesh->Positions.Num() * sizeof(FVector),
		JunMesh->Normals.GetData(), JunMesh->Normals.Num() * sizeof(FVector));
	FMemory::Memcpy(JunMeshBytes.GetData() + sizeof(int32) * 2 + (JunMesh->Positions.Num()+ JunMesh->Normals.Num()) * sizeof(FVector),
		JunMesh->Indices.GetData(), JunMesh->Indices.Num() * sizeof(uint32));
	
	bool bOk = FFileHelper::SaveArrayToFile(JunMeshBytes, *lPath);
	UE_LOG(LogTemp, Log, TEXT("[XSJ] Save %s to %s"), *Mesh->GetPathName(), *lPath);
#endif
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
}

void UAssetExporterBPLibrary::ExportMap(UWorld* World, const FString& Path)
{
	UE_LOG(LogTemp, Log, TEXT("RootPath = "), *Path);

	ns_yoyo::FLevelSceneInfo yySceneInfo;

	TSet<UStaticMesh*> ExportedStaticMeshes;

	ULevel* Level = World->PersistentLevel;
	for (AActor* Actor : Level->Actors)
	{
		// static mesh
		if (Actor->IsA(AStaticMeshActor::StaticClass()))
		{
			TArray<UActorComponent*> StaticMeshComponents =
				Actor->GetComponentsByClass(UStaticMeshComponent::StaticClass());
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
						GetAssetPath(StaticMesh);
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

	// write to file
	ns_yoyo::FLevelResource yyLevelResource;
	yyLevelResource.Path = GetAssetPath(Level);
	FString SavePath = Path + yyLevelResource.Path;
	yyLevelResource.SceneInfo = yySceneInfo;

	TArray<uint8> ByteData;
	FMemoryWriter BytesWriter(ByteData);
	BytesWriter << yyLevelResource;

	bool bOk = FFileHelper::SaveArrayToFile(ByteData, *SavePath);
	check(bOk);
}

