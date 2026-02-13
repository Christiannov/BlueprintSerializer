#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "BSDataTypes.generated.h"

// Log category declared in BlueprintSerializerModule.h

/**
 * Export mode for Blueprint serialization
 */
UENUM(BlueprintType)
enum class EBSExportMode : uint8
{
	ProjectSaved    UMETA(DisplayName = "Project Saved Directory"),
	EngineSaved     UMETA(DisplayName = "Engine Saved Directory"),
	Documents       UMETA(DisplayName = "User Documents"),
	Temporary       UMETA(DisplayName = "Temporary Directory"),
	Custom          UMETA(DisplayName = "Custom Path")
};

/**
 * Serialization depth for controlling data detail level
 */
UENUM(BlueprintType)
enum class EBSSerializationDepth : uint8
{
	Minimal         UMETA(DisplayName = "Minimal - Core Data Only"),
	Standard        UMETA(DisplayName = "Standard - Include Metadata"),
	Extended        UMETA(DisplayName = "Extended - Include Graph Structure"),
	Complete        UMETA(DisplayName = "Complete - All Available Data")
};

/**
 * Pin data structure for Blueprint node pins
 */
USTRUCT(BlueprintType)
struct FBSPinData
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString PinId;
	
	UPROPERTY()
	FString PinName;
	
	UPROPERTY()
	FString PinType;
	
	UPROPERTY()
	FString SubType;
	
	UPROPERTY()
	FString ObjectType;
	
	UPROPERTY()
	FString DefaultValue;
	
	UPROPERTY()
	EPinContainerType ContainerType = EPinContainerType::None;
	
	UPROPERTY()
	bool bIsReference = false;
	
	UPROPERTY()
	bool bIsConst = false;
	
	UPROPERTY()
	bool bIsOut = false;
	
	UPROPERTY()
	bool bIsExec = false;
	
	UPROPERTY()
	TArray<FString> LinkedTo;
};

/**
 * Node data structure for Blueprint graph nodes
 */
USTRUCT(BlueprintType)
struct FBSNodeData
{
	GENERATED_BODY()
	
	UPROPERTY()
	FGuid NodeGuid;
	
	UPROPERTY()
	FString NodeType;
	
	UPROPERTY()
	FString NodeClass;
	
	UPROPERTY()
	FString NodeTitle;
	
	UPROPERTY()
	FString FunctionName;
	
	UPROPERTY()
	FString MemberParent;
	
	UPROPERTY()
	FString Comment;
	
	UPROPERTY()
	FVector2D NodePosition;
	
	UPROPERTY()
	TArray<FBSPinData> InputPins;
	
	UPROPERTY()
	TArray<FBSPinData> OutputPins;
	
	UPROPERTY()
	TMap<FString, FString> Metadata;
	
	UPROPERTY()
	bool bIsPure = false;
	
	UPROPERTY()
	bool bIsConst = false;
	
	UPROPERTY()
	bool bIsLatent = false;
};

/**
 * Flow edge for execution connections
 */
USTRUCT(BlueprintType)
struct FBSFlowEdge
{
	GENERATED_BODY()
	
	UPROPERTY()
	FGuid SourceNodeGuid;
	
	UPROPERTY()
	FString SourcePinName;
	
	UPROPERTY()
	FGuid TargetNodeGuid;
	
	UPROPERTY()
	FString TargetPinName;
};

/**
 * Graph data structure for Blueprint graphs
 */
USTRUCT(BlueprintType)
struct FBSGraphData
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString GraphName;
	
	UPROPERTY()
	FString GraphType;
	
	UPROPERTY()
	FString GraphCategory;
	
	UPROPERTY()
	TArray<FBSNodeData> Nodes;
	
	UPROPERTY()
	TArray<FBSFlowEdge> ExecutionFlows;
	
	UPROPERTY()
	TMap<FString, FString> DataFlows;
	
	UPROPERTY()
	TMap<FString, FString> GraphMetadata;
	
	UPROPERTY()
	int32 NodeCount = 0;
};

/**
 * Variable information
 */
USTRUCT(BlueprintType)
struct FBSVariableInfo
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	FString Type;
	
	UPROPERTY()
	FString Category;
	
	UPROPERTY()
	FString DefaultValue;
	
	UPROPERTY()
	bool bIsPublic = false;
	
	UPROPERTY()
	bool bIsBlueprintReadOnly = false;
	
	UPROPERTY()
	bool bIsEditableOnInstance = true;
	
	UPROPERTY()
	bool bIsReplicatedVariable = false;
	
	UPROPERTY()
	FString Tooltip;
};

/**
 * Function information
 */
USTRUCT(BlueprintType)
struct FBSFunctionInfo
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	FString Category;
	
	UPROPERTY()
	FString AccessSpecifier = TEXT("Public");
	
	UPROPERTY()
	TArray<FBSPinData> InputParameters;
	
	UPROPERTY()
	TArray<FBSPinData> OutputParameters;
	
	UPROPERTY()
	bool bIsPure = false;
	
	UPROPERTY()
	bool bIsConst = false;
	
	UPROPERTY()
	bool bIsStatic = false;
	
	UPROPERTY()
	bool bIsOverride = false;
	
	UPROPERTY()
	bool bIsEvent = false;
	
	UPROPERTY()
	bool bIsCallInEditor = false;
	
	UPROPERTY()
	FString Tooltip;
};

/**
 * Component information
 */
USTRUCT(BlueprintType)
struct FBSComponentInfo
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString Name;
	
	UPROPERTY()
	FString ComponentClass;
	
	UPROPERTY()
	FString ParentComponent;
	
	UPROPERTY()
	FTransform RelativeTransform;
	
	UPROPERTY()
	TMap<FString, FString> Properties;
	
	UPROPERTY()
	bool bIsRootComponent = false;
	
	UPROPERTY()
	bool bIsInherited = false;
};

/**
 * Main Blueprint data structure
 */
USTRUCT(BlueprintType)
struct FBSBlueprintData
{
	GENERATED_BODY()
	
	// Basic information
	UPROPERTY()
	FString BlueprintPath;
	
	UPROPERTY()
	FString BlueprintName;
	
	UPROPERTY()
	FString ParentClassName;
	
	UPROPERTY()
	FString GeneratedClassName;
	
	UPROPERTY()
	TArray<FString> ImplementedInterfaces;
	
	// Blueprint type flags
	UPROPERTY()
	bool bIsInterface = false;
	
	UPROPERTY()
	bool bIsMacroLibrary = false;
	
	UPROPERTY()
	bool bIsFunctionLibrary = false;
	
	UPROPERTY()
	bool bIsLevelScript = false;
	
	// Data collections
	UPROPERTY()
	TArray<FBSVariableInfo> Variables;
	
	UPROPERTY()
	TArray<FBSFunctionInfo> Functions;
	
	UPROPERTY()
	TArray<FBSComponentInfo> Components;
	
	UPROPERTY()
	TArray<FBSGraphData> Graphs;
	
	// Metadata
	UPROPERTY()
	FDateTime ExtractionTimestamp;
	
	UPROPERTY()
	FString EngineVersion;
	
	UPROPERTY()
	FString PluginVersion = TEXT("1.0.0");
	
	UPROPERTY()
	FString ProjectName;
	
	UPROPERTY()
	int32 TotalNodeCount = 0;
	
	UPROPERTY()
	TMap<FString, FString> BlueprintMetadata;
};

/**
 * Asset reference data for dependency tracking
 */
USTRUCT(BlueprintType)
struct FBSAssetReference
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString AssetPath;
	
	UPROPERTY()
	FString AssetType;
	
	UPROPERTY()
	FString ReferenceType;
	
	UPROPERTY()
	TArray<FString> ReferencedBy;
};

/**
 * Dependency graph data
 */
USTRUCT(BlueprintType)
struct FBSDependencyGraph
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString RootAsset;
	
	UPROPERTY()
	TArray<FBSAssetReference> Dependencies;
	
	UPROPERTY()
	TArray<FBSAssetReference> Referencers;
	
	UPROPERTY()
	int32 TotalDependencies = 0;
	
	UPROPERTY()
	int32 TotalReferencers = 0;
};