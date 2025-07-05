#pragma once
#include "BaseMatchView.h"

class BaseMainView
{
public:
	std::function<void()> ToolButtonClicked;
	std::function<void()> ResetButtonClicked;
	std::function<void()> BoundsChanged;

	virtual BaseMatchView* getMatchView() = 0;
};