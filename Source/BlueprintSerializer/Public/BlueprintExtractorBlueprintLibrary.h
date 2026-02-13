#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Blueprint.h"
#include "BlueprintAnalyzer.h"
#include "DataExportManager.h"
#include "BlueprintExtractorBlueprintLibrary.generated.h"

/**
 * Blueprint Function Library for Blueprint Serializer
 * Provides Blueprint-accessible functions for serialization operations
 * Can be called from Blueprint graphs for integration with visual scripting
 */
UCLASS()
class BLUEPRINTSERIALIZER_API UBlueprintSerializerBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Serialize all project Blueprint data to JSON
	 * Returns true if successful, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static bool SerializeAllProjectBlueprints();

	/**
	 * Export all project Blueprint data (legacy name)
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static bool ExportAllProjectBlueprintData();

	/**
	 * Get count of Blueprints in project
	 */
	UFUNCTION(BlueprintPure, Category = "Blueprint Serializer", meta = (DevelopmentOnly = "true"))
	static int32 GetProjectBlueprintCount();

	/**
	 * Serialize a specific Blueprint and return JSON string
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static FString SerializeSingleBlueprint(UBlueprint* TargetBlueprint);

	/**
	 * Analyze single Blueprint (legacy name)
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static FString AnalyzeSingleBlueprint(UBlueprint* TargetBlueprint);

	/**
	 * Export complete project data (Blueprints + Dependencies)
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static bool ExportCompleteProjectData();

	/**
	 * Get the export directory path from settings
	 */
	UFUNCTION(BlueprintPure, Category = "Blueprint Serializer")
	static FString GetExportDirectory();
	
	/**
	 * Generate LLM context for a Blueprint
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor)
	static bool GenerateLLMContext(UBlueprint* TargetBlueprint);

	/**
	 * Audit a Blueprint for structural coverage + bytecode hashes and write a report JSON.
	 * Returns the full path to the report on success, empty string on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static FString AuditSingleBlueprintToFile(UBlueprint* TargetBlueprint);

	/**
	 * Round-trip audit: serialize -> rebuild transient Blueprint -> compile -> compare bytecode hashes.
	 * Returns the full path to the round-trip audit report JSON, empty string on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Blueprint Serializer", CallInEditor, meta = (DevelopmentOnly = "true"))
	static FString RoundTripAuditSingleBlueprint(UBlueprint* TargetBlueprint);
};
