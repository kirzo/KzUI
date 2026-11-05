// Copyright 2026 kirzo

#include "KzUIIconSource.h"
#include "KzInputIcon.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUserWidget.h"

#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"

namespace
{
	/** One default instance per source type, in token order. Rebuilt after modules come or go, which can bring new types. */
	const TArray<TInstancedStruct<FKzUIIconSource>>& GetTokenSources()
	{
		static TArray<TInstancedStruct<FKzUIIconSource>> Sources;
		static FDelegateHandle ModulesChangedHandle;
		if (!ModulesChangedHandle.IsValid())
		{
			ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddLambda([](FName, EModuleChangeReason) { Sources.Reset(); });
		}

		if (Sources.IsEmpty())
		{
			const UScriptStruct* Base = FKzUIIconSource::StaticStruct();
			for (TObjectIterator<UScriptStruct> It; It; ++It)
			{
				if (*It != Base && It->IsChildOf(Base) && !(It->StructFlags & STRUCT_NewerVersionExists))
				{
					TInstancedStruct<FKzUIIconSource>& Source = Sources.AddDefaulted_GetRef();
					Source.InitializeAsScriptStruct(*It);
				}
			}
			Sources.StableSort([](const TInstancedStruct<FKzUIIconSource>& A, const TInstancedStruct<FKzUIIconSource>& B) { return A.Get().GetTokenOrder() < B.Get().GetTokenOrder(); });
		}
		return Sources;
	}
}

TInstancedStruct<FKzUIIconSource> FKzUIIconSource::MakeFromToken(const FString& Token, const UKzUIIconTheme* Theme)
{
	for (const TInstancedStruct<FKzUIIconSource>& Type : GetTokenSources())
	{
		TInstancedStruct<FKzUIIconSource> Source = Type;
		if (Source.GetMutable().InitFromToken(Token, Theme))
		{
			return Source;
		}
	}
	return TInstancedStruct<FKzUIIconSource>();
}

FKey FKzUIIconSource::FindKeyForDevice(TConstArrayView<FKey> Keys, EKzUIInputDevice Device)
{
	const bool bGamepad = Device != EKzUIInputDevice::Keyboard;
	for (const FKey& Key : Keys)
	{
		if (Key.IsGamepadKey() == bGamepad)
		{
			return Key;
		}
	}
	return FKey();
}

FKzUIIconResolution FKzUIIconSource_Input::Resolve(const FKzUIIconContext& Context) const
{
	FKzUIIconResolution Resolution;
	Resolution.Icon = Context.Theme ? Context.Theme->GetIcon(Input, Context.Device) : nullptr;
	if (!Resolution.Icon)
	{
		const FKzUIInputDefinition* Definition = UKzUIInputSettings::Get()->FindInputDefinition(Input, Context.GamepadDevice);
		Resolution.Key = Definition ? FindKeyForDevice(Definition->GetKeys(), Context.Device) : FKey();
	}
	return Resolution;
}

float FKzUIIconSource_Input::GetHoldProgress(const FKzUIIconContext& Context) const
{
	const UKzUserWidget* Owner = Context.Icon ? Context.Icon->GetTypedOuter<UKzUserWidget>() : nullptr;
	return Owner ? Owner->GetInputHoldProgress(Input) : 0.0f;
}

bool FKzUIIconSource_Input::InitFromToken(const FString& Token, const UKzUIIconTheme* Theme)
{
	const int64 Value = StaticEnum<EKzUIInputType>()->GetValueByNameString(Token);
	if (Value <= (int64)EKzUIInputType::None || Value >= (int64)EKzUIInputType::MAX)
	{
		return false;
	}
	Input = (EKzUIInputType)Value;
	return true;
}

FKzUIIconResolution FKzUIIconSource_Custom::Resolve(const FKzUIIconContext& Context) const
{
	FKzUIIconResolution Resolution;
	Resolution.Icon = Context.Theme ? Context.Theme->GetCustomIcon(Token, Context.Device) : nullptr;
	return Resolution;
}

bool FKzUIIconSource_Custom::InitFromToken(const FString& InToken, const UKzUIIconTheme* Theme)
{
	if (!Theme || !Theme->HasCustomIcon(FName(*InToken)))
	{
		return false;
	}
	Token = FName(*InToken);
	return true;
}

FKzUIIconResolution FKzUIIconSource_Key::Resolve(const FKzUIIconContext& Context) const
{
	FKzUIIconResolution Resolution;
	Resolution.Key = Key;
	return Resolution;
}

bool FKzUIIconSource_Key::InitFromToken(const FString& Token, const UKzUIIconTheme* Theme)
{
	const FKey Parsed(*Token);
	if (!Parsed.IsValid())
	{
		return false;
	}
	Key = Parsed;
	return true;
}