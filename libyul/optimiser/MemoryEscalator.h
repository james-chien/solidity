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
/**
 * Optimisation stage that escalates local solidity variables to memory to avoid "stack too deep"-errors.
 * TODO: elaborate on scope and mechanism.
 */

#pragma once

#include <libyul/optimiser/OptimiserStep.h>

namespace solidity::yul
{

struct Object;

/**
 * TODO
 *
 * Prerequisite: Disambiguator, Function Grouper, TODO
 */
class MemoryEscalator
{
public:
	static bool run(
		OptimiserStepContext& _context,
		Object& _object,
		bool _optimizeStackAllocation
	);
	static void runUntilStabilized(
		OptimiserStepContext& _context,
		Object& _object,
		bool _optimizeStackAllocation
	);
};

}
