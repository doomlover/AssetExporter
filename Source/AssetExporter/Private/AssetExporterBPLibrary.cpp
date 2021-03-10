// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetExporterBPLibrary.h"
#include "AssetExporter.h"
#include "JsonObjectConverter.h"
#include "JunJsonAsset.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#if 0
#include "StaticMeshAttributes.h"
#include "MeshElementArray.h"
#endif


UAssetExporterBPLibrary::UAssetExporterBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float UAssetExporterBPLibrary::AssetExporterSampleFunction(float Param)
{
	return -1;
}

void UAssetExporterBPLibrary::ExportStaticMesh(UStaticMesh* Mesh, const FString& Path)
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

void UAssetExporterBPLibrary::ExportCamera(ACameraActor* Camera, const FString& Path)
{
	check(Camera);
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
}

