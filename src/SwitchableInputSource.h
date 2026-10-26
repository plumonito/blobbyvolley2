/*=============================================================================
Blobby Volley 2
Copyright (C) 2026 Blobby Volley 2 contributors

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <memory>

#include "InputSource.h"

/*! \class SwitchableInputSource
	\brief forwards the input of another input source, which can be exchanged at any time
	\details Used when several players take turns at controlling the same blob.
*/
class SwitchableInputSource : public InputSource
{
	public:
		SwitchableInputSource();
		~SwitchableInputSource() override;

		void setSource(std::shared_ptr<InputSource> source);

	private:
		PlayerInputAbs getNextInput() override;

		std::shared_ptr<InputSource> mSource;
};
