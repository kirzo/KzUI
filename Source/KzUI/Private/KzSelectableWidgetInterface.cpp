// Copyright 2026 kirzo

#include "KzSelectableWidgetInterface.h"
#include "Components/Widget.h"

namespace
{
	enum class EKzSelectableEvent : uint8 { Select, Hover };

	void Propagate(UWidget* Widget, EKzSelectableEvent Event, bool bValue, TSet<UWidget*>& Visited)
	{
		if (!Widget || Visited.Contains(Widget) || !Widget->Implements<UKzSelectableWidgetInterface>())
		{
			return;
		}
		Visited.Add(Widget);

		if (Event == EKzSelectableEvent::Select)
		{
			bValue ? IKzSelectableWidgetInterface::Execute_OnSelect(Widget) : IKzSelectableWidgetInterface::Execute_OnDeselect(Widget);
		}
		else
		{
			bValue ? IKzSelectableWidgetInterface::Execute_OnHovered(Widget) : IKzSelectableWidgetInterface::Execute_OnUnhovered(Widget);
		}

		for (UWidget* Linked : IKzSelectableWidgetInterface::Execute_GetLinkedSelectables(Widget))
		{
			Propagate(Linked, Event, bValue, Visited);
		}
	}
}

void IKzSelectableWidgetInterface::Select(UWidget* Widget, bool bSelected)
{
	TSet<UWidget*> Visited;
	Propagate(Widget, EKzSelectableEvent::Select, bSelected, Visited);
}

void IKzSelectableWidgetInterface::Hover(UWidget* Widget, bool bHovered)
{
	TSet<UWidget*> Visited;
	Propagate(Widget, EKzSelectableEvent::Hover, bHovered, Visited);
}