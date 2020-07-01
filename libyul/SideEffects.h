/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <set>

namespace solidity::yul
{

/**
 * Side effects of code.
 *
 * The default-constructed value applies to the "empty code".
 */
struct SideEffects
{
	/// Corresponds to the effect that a YUL-builtin has on a generic data location (storage, memory
	/// and other blockchain state.)
	enum Effect
	{
		None,
		Read,
		Write
	};

	friend Effect operator+(Effect const& _a, Effect const& _b)
	{
		return static_cast<Effect>(std::max(static_cast<int>(_a), static_cast<int>(_b)));
	}

	/// If true, expressions in this code can be freely moved and copied without altering the
	/// semantics.
	/// At statement level, it means that functions containing this code can be
	/// called multiple times, their calls can be rearranged and calls can also be
	/// deleted without changing the semantics.
	/// This means it cannot depend on storage or memory, cannot have any side-effects,
	/// but it can depend on state that is constant across an EVM-call.
	bool movable = true;
	/// If true, the code can be removed without changing the semantics.
	bool sideEffectFree = true;
	/// If true, the code can be removed without changing the semantics as long as
	/// the whole program does not contain the msize instruction.
	bool sideEffectFreeIfNoMSize = true;
	/// If false, the execution can potentially go in an infinite loop.
	bool cannotLoop = true;
	Effect otherState = None;
	Effect storage = None;
	Effect memory = None;

	/// @returns the worst-case side effects.
	static SideEffects worst()
	{
		return SideEffects{false, false, false, false, Write, Write, Write};
	}

	/// @returns the combined side effects of two pieces of code.
	SideEffects operator+(SideEffects const& _other)
	{
		return SideEffects{
			movable && _other.movable,
			sideEffectFree && _other.sideEffectFree,
			sideEffectFreeIfNoMSize && _other.sideEffectFreeIfNoMSize,
			cannotLoop && _other.cannotLoop,
			otherState + _other.otherState,
			storage + _other.storage,
			memory +  _other.memory,
		};
	}

	/// Adds the side effects of another piece of code to this side effect.
	SideEffects& operator+=(SideEffects const& _other)
	{
		*this = *this + _other;
		return *this;
	}

	bool operator==(SideEffects const& _other) const
	{
		return
			movable == _other.movable &&
			sideEffectFree == _other.sideEffectFree &&
			sideEffectFreeIfNoMSize == _other.sideEffectFreeIfNoMSize &&
			otherState == _other.otherState &&
			storage == _other.storage &&
			memory == _other.memory;
	}
};

}
