/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "State.h"

/* includes */
#include <cassert>

#include "BlobbyApp.h"
#include "SoundManager.h"
#include "IMGUI.h"
#include "InputManager.h"
#include "RenderManager.h"
#include "Theme.h"

/* implementation */

State::State() = default;

void State::playSound(const std::string& sound, float volume) {
	assert(m_App);
	m_App->getSoundManager().playSound(sound, volume);
}

bool State::is_exiting() const {
	return getInputMgr().exit();
}

void State::doHints(float y, const std::vector<Hint>& hints)
{
	const float buttonSize = 28;
	const float gap = 8;
	const float spacing = 26;
	RenderManager& renderer = getRenderer();

	// measure the row first, to centre it
	std::vector<float> buttonWidths;
	float width = 0;
	for (const auto& hint : hints)
	{
		float buttonWidth = hint.button.size() == 1 ? buttonSize : renderer.getTextWidth(hint.button, TF_SMALL_FONT) + 20;
		buttonWidths.push_back(buttonWidth);
		width += buttonWidth + gap + renderer.getTextWidth(hint.label, TF_SMALL_FONT) + spacing;
	}
	float x = 400 - (width - spacing) / 2;

	IMGUI& imgui = getIMGUI();
	for (unsigned i = 0; i < hints.size(); ++i)
	{
		const Hint& hint = hints[i];
		float alpha = hint.enabled ? 1 : 0.35;
		imgui.doPanel(Vector2(x, y), Vector2(x + buttonWidths[i], y + buttonSize), hint.color, alpha, buttonSize / 2);
		imgui.doText(Vector2(x + buttonWidths[i] / 2, y + 2), hint.button, TF_SMALL_FONT | TF_ALIGN_CENTER,
				hint.enabled ? theme::TEXT : theme::TEXT_DIM);
		x += buttonWidths[i] + gap;
		imgui.doText(Vector2(x, y + 2), hint.label, TF_SMALL_FONT, hint.enabled ? theme::TEXT : theme::TEXT_DIM);
		x += renderer.getTextWidth(hint.label, TF_SMALL_FONT) + spacing;
	}
}

void State::setApp(BlobbyApp* app) {
	assert(app);
	m_App = app;
}

void State::switchState(State* newState)
{
	assert(m_App);
	m_App->switchToState(std::unique_ptr<State>(newState));
}

BlobbyApp& State::getApp() const
{
	assert(m_App);
	return *m_App;
}

IMGUI& State::getIMGUI() const {
	return getApp().getIMGUI();
}

InputManager& State::getInputMgr() const {
	return getApp().getInputManager();
}

RenderManager& State::getRenderer() const {
	return getApp().getRenderManager();
}
