// Copyright 2026 kirzo

#include "KzTextBlock.h"
#include "KzInputIcon.h"
#include "KzTextStyle.h"
#include "KzUIIconTheme.h"
#include "KzUIInputSettings.h"
#include "KzUIInputSubsystem.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/BreakIterator.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
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
}

void UKzTextBlock::SetText(FText InText)
{
	Super::SetText(InText);

	// The text it already shows rebuilds nothing, so icon handles and their materials survive
	if (GetText().ToString() != BuiltSource)
	{
		RebuildContent();
	}
}

void UKzTextBlock::SetIsEnabled(bool bInIsEnabled)
{
	const bool bChanged = GetIsEnabled() != bInIsEnabled;
	Super::SetIsEnabled(bInIsEnabled);

	if (bChanged)
	{
		ApplyStyle();
	}
}

void UKzTextBlock::OnSelect_Implementation()
{
	bSelected = true;
	ApplyStyle();
}

void UKzTextBlock::OnDeselect_Implementation()
{
	bSelected = false;
	ApplyStyle();
}

void UKzTextBlock::OnHovered_Implementation()
{
	bHovered = true;
	ApplyStyle();
}

void UKzTextBlock::OnUnhovered_Implementation()
{
	bHovered = false;
	ApplyStyle();
}

void UKzTextBlock::SynchronizeProperties()
{
	ApplyStyle();
	Super::SynchronizeProperties();
	RebuildContent();
}

void UKzTextBlock::OnJustificationChanged(ETextJustify::Type InJustification)
{
	Super::OnJustificationChanged(InJustification);
	RebuildContent();
}

void UKzTextBlock::OnAutoWrapTextChanged(bool InAutoWrapText)
{
	Super::OnAutoWrapTextChanged(InAutoWrapText);
	RebuildContent();
}

void UKzTextBlock::OnWrapTextAtChanged(float InWrapTextAt)
{
	Super::OnWrapTextAtChanged(InWrapTextAt);
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
	BuiltSource.Reset();

	FTextLocalizationManager::Get().OnTextRevisionChangedEvent.Remove(TextRevisionHandle);
	TextRevisionHandle.Reset();

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
	const UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::GetForWidget(this);
	return Subsystem ? Subsystem->GetIconTheme() : UKzUIInputSettings::Get()->IconTheme.LoadSynchronous();
}

float UKzTextBlock::GetLineHeight() const
{
	return FSlateApplication::IsInitialized() ? FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->GetMaxCharacterHeight(GetFont()) : 0.0f;
}

void UKzTextBlock::OnInputDeviceChanged(EKzUIInputDevice InputDevice)
{
	if (GetActiveIconTheme() != BuiltTheme.Get())
	{
		RebuildContent();
	}
}

void UKzTextBlock::OnTextRevisionChanged()
{
	// The words are baked strings, so a new culture needs a new build
	RebuildContent();
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

	if (!TextRevisionHandle.IsValid())
	{
		TextRevisionHandle = FTextLocalizationManager::Get().OnTextRevisionChangedEvent.AddUObject(this, &UKzTextBlock::OnTextRevisionChanged);
	}

	// The content is built by SynchronizeProperties, which TakeWidget calls right after this
	const TWeakObjectPtr<UKzTextBlock> WeakThis(this);
	return SNew(SBox)
		.Padding_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->Margin : FMargin(); })
		[
			MyBox.ToSharedRef()
		];
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
	bNextOnNewLine = false;
	bHasSlots = false;

	// Custom tokens are recognized against the theme, so a theme swap must rebuild
	if (!IsDesignTime())
	{
		UKzUIInputSubsystem* Subsystem = UKzUIInputSubsystem::GetForWidget(this);
		if (Subsystem != BoundSubsystem.Get())
		{
			if (UKzUIInputSubsystem* Previous = BoundSubsystem.Get())
			{
				Previous->OnInputDeviceChanged.RemoveDynamic(this, &UKzTextBlock::OnInputDeviceChanged);
			}
			if (Subsystem)
			{
				Subsystem->OnInputDeviceChanged.AddUniqueDynamic(this, &UKzTextBlock::OnInputDeviceChanged);
			}
			BoundSubsystem = Subsystem;
		}
	}

	const UKzUIIconTheme* Theme = GetActiveIconTheme();
	BuiltTheme = Theme;

	const bool bLeadingSpaces = Justification == ETextJustify::Right;
	const FString Source = GetText().ToString();
	BuiltSource = Source;
	FString Pending;

	for (int32 i = 0; i < Source.Len(); ++i)
	{
		if (Source[i] == TEXT('\r'))
		{
			continue;
		}

		if (Source[i] == TEXT('\n'))
		{
			AddTextSegment(Pending);

			// A paragraph with nothing in it still takes its line
			if (bNextOnNewLine)
			{
				AddTextBlock(FString());
			}
			bNextOnNewLine = true;
			continue;
		}

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

	// Empty text and a trailing line break still take a line, like a plain TextBlock
	if (!bHasSlots || bNextOnNewLine)
	{
		AddTextBlock(FString());
	}
}

bool UKzTextBlock::TakeNewLine()
{
	const bool bNewLine = bNextOnNewLine;
	bNextOnNewLine = false;
	bHasSlots = true;
	return bNewLine;
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

	// One block per break opportunity of the engine's line breaker, so the wrap can break there; languages
	// written without spaces break between characters. Each piece carries its trailing spaces.
	if (!LineBreaker.IsValid())
	{
		LineBreaker = FBreakIterator::CreateLineBreakIterator();
	}
	TArray<FString> Pieces;
	LineBreaker->SetString(Pending);
	LineBreaker->ResetToBeginning();
	int32 Start = 0;
	for (int32 Break = LineBreaker->MoveToNext(); Break != INDEX_NONE; Break = LineBreaker->MoveToNext())
	{
		if (Break > Start)
		{
			Pieces.Add(Pending.Mid(Start, Break - Start));
			Start = Break;
		}
	}
	if (Start < Pending.Len())
	{
		Pieces.Add(Pending.Mid(Start));
	}
	LineBreaker->ClearString();

	// Spaces go to the side of the ragged edge, where leftovers are invisible: leading for right justification
	if (Justification == ETextJustify::Right)
	{
		for (int32 Index = 0; Index + 1 < Pieces.Num(); ++Index)
		{
			const FString Trimmed = Pieces[Index].TrimEnd();
			Pieces[Index + 1] = Pieces[Index].RightChop(Trimmed.Len()) + Pieces[Index + 1];
			Pieces[Index] = Trimmed;
		}
	}

	for (const FString& Piece : Pieces)
	{
		if (!Piece.IsEmpty())
		{
			AddTextBlock(Piece);
		}
	}
	Pending.Reset();
}

void UKzTextBlock::AddTextBlock(const FString& Word)
{
	// Appearance is read live, so setters, styles and UMG animations apply without a rebuild
	const TWeakObjectPtr<UKzTextBlock> WeakThis(this);
	MyBox->AddSlot()
	.ForceNewLine(TakeNewLine())
	.VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Word))
		.Font_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetFont() : FSlateFontInfo(); })
		.ColorAndOpacity_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetColorAndOpacity() : FSlateColor(); })
		.ShadowOffset_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetShadowOffset() : FVector2D::ZeroVector; })
		.ShadowColorAndOpacity_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetShadowColorAndOpacity() : FLinearColor::Transparent; })
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
	GeneratedIcons.Add(Icon);

	const TWeakObjectPtr<UKzTextBlock> WeakThis(this);
	const TWeakObjectPtr<UKzInputIcon> WeakIcon(Icon);
	TSharedRef<SWidget> Content = Icon->TakeWidget();

	if (bAutoIconSize)
	{
		// The line height and the icon's own proportions, read live: a style with another font or a device
		// with a wider icon (Space, LT+RT) resizes without a rebuild
		Content = SNew(SBox)
			.HeightOverride_Lambda([WeakThis]()
				{
					return FOptionalSize(WeakThis.IsValid() ? WeakThis->GetLineHeight() : 0.0f);
				})
			.WidthOverride_Lambda([WeakThis, WeakIcon]()
				{
					const float Height = WeakThis.IsValid() ? WeakThis->GetLineHeight() : 0.0f;
					const FVector2D Natural = WeakIcon.IsValid() ? WeakIcon->GetNaturalSize() : FVector2D::ZeroVector;
					return FOptionalSize(Natural.X > 0.0 && Natural.Y > 0.0 ? Height * float(Natural.X / Natural.Y) : Height);
				})
			[
				Content
			];
	}
	else if (IconSize.X > 0.0 && IconSize.Y > 0.0)
	{
		Content = SNew(SBox)
			.WidthOverride(IconSize.X)
			.HeightOverride(IconSize.Y)
			[
				Content
			];
	}

	if (bApplyColorToIcons)
	{
		Content = SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
			.Padding(0.0f)
			.ColorAndOpacity_Lambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::White; })
			[
				Content
			];
	}

	// Spaces become padding measured in the live font
	MyBox->AddSlot()
	.ForceNewLine(TakeNewLine())
	.Padding(TAttribute<FMargin>::CreateLambda([WeakThis, SpacesBefore, SpacesAfter]()
		{
			const float SpaceWidth = WeakThis.IsValid() && FSlateApplication::IsInitialized() && (SpacesBefore > 0 || SpacesAfter > 0)
				? FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(TEXT(" "), WeakThis->GetFont()).X
				: 0.0f;
			return FMargin(SpaceWidth * SpacesBefore, 0.0f, SpaceWidth * SpacesAfter, 0.0f);
		}))
	.VAlign(VAlign_Center)
	[
		Content
	];
}