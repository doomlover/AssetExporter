
#include "ExportTypes.h"
#include "Rendering/StaticMeshVertexBuffer.h"

/*
* code example

auto mesh_path = Mesh->GetPathName();
auto package_path = Mesh->GetPackage()->GetPathName();
auto package_file = FPackageName::LongPackageNameToFilename(Mesh->GetPackage()->GetPathName());
package_path.ReplaceInline(TEXT("/Game"), TEXT(""));
*/

void ns_yoyo::ExportVertexBuffer(FVertexBuffer& VertexBuffer, FStaticMeshVertexBuffers& VertexBuffers)
{
	VertexBuffer.NumVertices = VertexBuffers.PositionVertexBuffer.GetNumVertices();
	VertexBuffer.Stride = 32; // bytes
	auto& FloatRawData = VertexBuffer.RawData;
	for (uint32 i = 0; i < VertexBuffer.NumVertices; ++i)
	{
		// position
		auto Position = VertexBuffers.PositionVertexBuffer.VertexPosition(i);
		FloatRawData.Append({ Position.X, Position.Y, Position.Z });
		// normal
		auto Normal = VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(i);
		FloatRawData.Append({ Normal.X, Normal.Y, Normal.Z });
		// uv
		auto UV = VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
		FloatRawData.Append({ UV.X, UV.Y });
	}
}

void ns_yoyo::ExportStaticIndexBuffer(FIndexBuffer& yyIndexBuffer, FRawStaticIndexBuffer& ueIndexBuffer)
{
	yyIndexBuffer.NumIndices = ueIndexBuffer.GetNumIndices();
	ueIndexBuffer.GetCopy(yyIndexBuffer.BufferData);
}

void ns_yoyo::ExportMultiSizeIndexContainer(FIndexBuffer& yyIndexBuffer, FMultiSizeIndexContainer& ueIndexContainer)
{
	ueIndexContainer.GetIndexBuffer(yyIndexBuffer.BufferData);
	yyIndexBuffer.NumIndices = yyIndexBuffer.BufferData.Num();
}

