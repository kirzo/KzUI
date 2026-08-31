// Copyright 2026 kirzo

#include "KzTextBlock.h"
#include "KzInputIcon.h"
#include "KzTextStyle.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Text/STextBlock.h"

// Wrap width used to render as a single line: larger than any real allotment
static constexpr float KzNoWrapSize = 100000.0f;

static bool ParseInputToken(const FString& Token, EKzUIInputType& OutInput)
{
	const UEnum* Enum = StaticEnum<EKzUIInputType>();
	const int64 Value = Enum->GetValueByNameString(Token);
	if (Value <= (int64)EKzUIInputType::None || Value >= (int64)EKzUIInputType::MAX)
	{
		return false;
	}
	OutInput = (EKzUIInputType)Value;
	return true;
}

void UKzTextBlock::SetStyle(TSubclassOf<UKzTextBlockStyle> NewStyle)
{
	Style = NewStyle;
	ApplyStyle();
	RebuildContent();
}

void UKzTextBlock::SetText(FText InText)
{
	Super::SetText(InText);
	RebuildContent();
}

void UKzTextBlock::SetIsEnabled(bool bInIsEnabled)
{
	const bool bChanged = GetIsEnabled() != bInIsEnabled;
	Super::SetIsEnabled(bInIsEnabled);

	if (bChanged)
	{
		ApplyStyle();
		RebuildContent();
	}
}

void UKzTextBlock::OnSelect_Implementation()
{
	bSelected = true;
	ApplyStyle();
	RebuildContent();
}

void UKzTextBlock::OnDeselect_Implementation()
{
	bSelected = false;
	ApplyStyle();
	RebuildContent();
}

void UKzTextBlock::OnHovered_Implementation()
{
	bHovered = true;
	ApplyStyle();
	RebuildContent();
}

void UKzTextBlock::OnUnhovered_Implementation()
{
	bHovered = false;
	ApplyStyle();
	RebuildContent();
}

void UKzTextBlock::SynchronizeProperties()
{
	ApplyStyle();
	Super::SynchronizeProperties();
	RebuildContent();
}

void UKzTextBlock::ApplyStyle()
{
	// Hover is the live cursor, so it wins over the persistent selection while on top of it
	const EKzUIWidgetState State = !GetIsEnabled() ? EKzUIWidgetState::Disabled : (bHovered ? EKzUIWidgetState::Hovered : (bSelected ? EKzUIWidgetState::Selected : EKzUIWidgetState::Normal));

	const UKzTextStyle* Look = ResolveLook(State);
	if (!Look && State != EKzUIWidgetState::Normal)
	{
		Look = ResolveLook(EKzUIWidgetState::Normal);
	}
	if (Look)
	{
		SetFont(Look->Font);
		SetColorAndOpacity(Look->Color);
		SetShadowOffset(Look->ShadowOffset);
		SetShadowColorAndOpacity(Look->ShadowColor);
	}
}

const UKzTextStyle* UKzTextBlock::ResolveLook(EKzUIWidgetState State) const
{
	if (const TSubclassOf<UKzTextStyle>* Override = StateOverrides.Find(State))
	{
		if (*Override)
		{
			return GetDefault<UKzTextStyle>(*Override);
		}
	}

	if (Style)
	{
		const UKzTextBlockStyle* Set = GetDefault<UKzTextBlockStyle>(Style);
		TSubclassOf<UKzTextStyle> LookClass;
		switch (State)
		{
			case EKzUIWidgetState::Hovered:		LookClass = Set->Hovered; break;
			case EKzUIWidgetState::Selected:	LookClass = Set->Selected; break;
			case EKzUIWidgetState::Disabled:	LookClass = Set->Disabled; break;
			default:							LookClass = Set->Normal; break;
		}
		if (LookClass)
		{
			return GetDefault<UKzTextStyle>(LookClass);
		}
	}
	return nullptr;
}

void UKzTextBlock::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyBox.Reset();

	for (UKzInputIcon* Icon : GeneratedIcons)
	{
		if (Icon)
		{
			Icon->ReleaseSlateResources(bReleaseChildren);
		}
	}
	GeneratedIcons.Reset();
}

TArray<UKzInputIcon*> UKzTextBlock::GetIcons() const
{
	return ObjectPtrDecay(GeneratedIcons);
}

UKzInputIcon* UKzTextBlock::GetIcon(EKzUIInputType Input) const
{
	for (UKzInputIcon* Icon : GeneratedIcons)
	{
		if (Icon && Icon->CustomToken.IsNone() && Icon->Input == Input)
		{
			return Icon;
		}
	}
	return nullptr;
}

UKzInputIcon* UKzTextBlock::GetCustomIcon(FName Token) const
{
	for (UKzInputIcon* Icon : GeneratedIcons)
	{
		if (Icon && Icon->CustomToken == Token)
		{
			return Icon;
		}
	}
	return nullptr;
}

const UKzUIIconTheme* UKzTextBlock::GetActiveIconTheme() const
{
#if WITH_EDITOR
	if (IsDesignTime())
	{
		return UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
	}
#endif
	const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::Get(GetOwningLocalPlayer());
	return Subsystem ? Subsystem->GetIconTheme() : nullptr;
}

#if WITH_EDITOR
const FText UKzTextBlock::GetPaletteCategory()
{
	return INVTEXT("Kz UI");
}
#endif

TSharedRef<SWidget> UKzTextBlock::RebuildWidget()
{
	// The huge initial PreferredSize makes the FIRST measure a single line, so size-to-content
	// containers settle unwrapped instead of feeding the default 100px preferred width back
	// into their own size. When auto-wrapping, the allotted width takes over from there.
	MyBox = SNew(SWrapBox).UseAllottedSize(GetAutoWrapText()).PreferredSize(KzNoWrapSize);
	RebuildContent();
	return MyBox.ToSharedRef();
}

void UKzTextBlock::RebuildContent()
{
	if (!MyBox.IsValid())
	{
		return;
	}

	MyBox->SetUseAllottedSize(GetAutoWrapText());
	if (!GetAutoWrapText())
	{
		// Honor the fixed wrap width; otherwise reset whatever the allotted size left behind
		const float WrapAt = GetWrapTextAt();
		MyBox->SetWrapSize(WrapAt > 0.0f ? WrapAt : KzNoWrapSize);
	}

	MyBox->SetHorizontalAlignment(Justification == ETextJustify::Center ? HAlign_Center : (Justification == ETextJustify::Right ? HAlign_Right : HAlign_Left));

	MyBox->ClearChildren();
	GeneratedIcons.Reset();

	const UKzUIIconTheme* Theme = GetActiveIconTheme();
	const bool bLeadingSpaces = Justification == ETextJustify::Right;
	const FString Source = GetText().ToString();
	FString Pending;

	for (int32 i = 0; i < Source.Len(); ++i)
	{
		if (Source[i] == TEXT('{'))
		{
			const int32 End = Source.Find(TEXT("}"), ESearchCase::CaseSensitive, ESearchDir::FromStart, i + 1);
			if (End != INDEX_NONE)
			{
				FString Token = Source.Mid(i + 1, End - i - 1);

				// {Token:Variation} wraps the icon in a variation material of the theme
				FName Variation = NAME_None;
				FString TokenPart, VariationPart;
				if (Token.Split(TEXT(":"), &TokenPart, &VariationPart))
				{
					Token = TokenPart;
					Variation = FName(*VariationPart);
				}

				EKzUIInputType Input = EKzUIInputType::None;
				FName CustomToken = NAME_None;
				if (!ParseInputToken(Token, Input) && Theme && Theme->HasCustomIcon(FName(*Token)))
				{
					CustomToken = FName(*Token);
				}
				if (Input != EKzUIInputType::None || !CustomToken.IsNone())
				{
					// Spaces touching the icon on the side no word absorbs become slot padding,
					// so they travel with the icon instead of wrapping as standalone widgets
					int32 SpacesBefore = 0;
					if (bLeadingSpaces)
					{
						while (Pending.Len() > 0 && Pending[Pending.Len() - 1] == TEXT(' '))
						{
							Pending.LeftChopInline(1);
							++SpacesBefore;
						}
					}
					AddTextSegment(Pending);

					int32 SpacesAfter = 0;
					int32 Next = End + 1;
					if (!bLeadingSpaces)
					{
						while (Next < Source.Len() && Source[Next] == TEXT(' '))
						{
							++Next;
							++SpacesAfter;
						}
					}
					AddIcon(Input, CustomToken, Variation, SpacesBefore, SpacesAfter);
					i = Next - 1;
					continue;
				}
			}
		}
		Pending.AppendChar(Source[i]);
	}
	AddTextSegment(Pending);
}

void UKzTextBlock::AddTextSegment(FString& Pending)
{
	if (Pending.IsEmpty())
	{
		return;
	}

	if (!GetAutoWrapText() && GetWrapTextAt() <= 0.0f)
	{
		AddTextBlock(Pending);
		Pending.Reset();
		return;
	}

	// One block per word so the wrap can break between words. Spaces attach to the side of the
	// ragged edge, where leftovers are invisible: trailing for left/center, leading for right.
	const bool bLeadingSpaces = Justification == ETextJustify::Right;
	int32 Start = 0;
	while (Start < Pending.Len())
	{
		int32 End = Start;
		if (bLeadingSpaces)
		{
			while (End < Pending.Len() && Pending[End] == TEXT(' '))
			{
				++End;
			}
			while (End < Pending.Len() && Pending[End] != TEXT(' '))
			{
				++End;
			}
		}
		else
		{
			while (End < Pending.Len() && Pending[End] != TEXT(' '))
			{
				++End;
			}
			while (End < Pending.Len() && Pending[End] == TEXT(' '))
			{
				++End;
			}
		}
		AddTextBlock(Pending.Mid(Start, End - Start));
		Start = End;
	}
	Pending.Reset();
}

void UKzTextBlock::AddTextBlock(const FString& Word)
{
	MyBox->AddSlot()
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Word))
		.Font(GetFont())
		.ColorAndOpacity(GetColorAndOpacity())
		.ShadowOffset(GetShadowOffset())
		.ShadowColorAndOpacity(GetShadowColorAndOpacity())
		.TransformPolicy(GetTextTransformPolicy())
	];
}

void UKzTextBlock::AddIcon(EKzUIInputType Input, FName CustomToken, FName Variation, int32 SpacesBefore, int32 SpacesAfter)
{
	// The outer must be the WidgetTree: UWidget resolves its owning player through its direct outer
	UKzInputIcon* Icon = NewObject<UKzInputIcon>(GetOuter());
#if WITH_EDITOR
	// Generated widgets do not inherit the designer flags, and without them the icon resolves
	// through the (absent) game subsystem instead of the design-time preview path
	Icon->SetDesignerFlags(GetDesignerFlags());
#endif
	Icon->CustomToken = CustomToken;
	Icon->Variation = Variation;
	Icon->SetInput(Input);
	if (bApplyColorToIcons)
	{
		Icon->SetColorAndOpacity(GetColorAndOpacity().GetSpecifiedColor());
	}
	GeneratedIcons.Add(Icon);

	FVector2D EffectiveIconSize = IconSize;
	if (bAutoIconSize && FSlateApplication::IsInitialized())
	{
		const float LineHeight = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(GetFont());
		EffectiveIconSize = FVector2D(LineHeight, LineHeight);
	}

	TSharedRef<SWidget> Content = Icon->TakeWidget();
	if (EffectiveIconSize.X > 0.0 && EffectiveIconSize.Y > 0.0)
	{
		Content = SNew(SBox)
			.WidthOverride(EffectiveIconSize.X)
			.HeightOverride(EffectiveIconSize.Y)
			[
				Content
			];
	}

	float SpaceWidth = 0.0f;
	if ((SpacesBefore > 0 || SpacesAfter > 0) && FSlateApplication::IsInitialized())
	{
		SpaceWidth = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT(" "), GetFont()).X;
	}

	MyBox->AddSlot()
	.Padding(FMargin(SpaceWidth * SpacesBefore, 0.0f, SpaceWidth * SpacesAfter, 0.0f))
	.VAlign(VAlign_Center)
	[
		Content
	];
}