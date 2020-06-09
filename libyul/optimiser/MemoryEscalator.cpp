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

#include <libyul/optimiser/MemoryEscalator.h>
#include <libyul/optimiser/CallGraphGenerator.h>
#include <libyul/optimiser/ExpressionJoiner.h>
#include <libyul/optimiser/ExpressionSplitter.h>
#include <libyul/optimiser/NameDispenser.h>
#include <libyul/backends/evm/EVMDialect.h>
#include <libyul/AsmData.h>
#include <libyul/CompilabilityChecker.h>
#include <libyul/Dialect.h>
#include <libyul/Exceptions.h>
#include <libyul/Object.h>
#include <libyul/Utilities.h>
#include <libsolutil/Algorithms.h>
#include <libsolutil/CommonData.h>
#include <libsolutil/Visitor.h>
#include <list>

using namespace std;
using namespace solidity;
using namespace solidity::yul;

namespace {
class MemoryInitFinder: ASTModifier
{
public:
	static FunctionCall* run(Block& _block)
	{
		MemoryInitFinder memoryInitFinder;
		memoryInitFinder(_block);
		return memoryInitFinder.m_memoryInit;
	}
	using ASTModifier::operator();
	void operator()(FunctionCall& _functionCall) override
	{
		ASTModifier::operator()(_functionCall);
		if (_functionCall.functionName.name == "memoryinit"_yulstring)
		{
			yulAssert(!m_memoryInit, "More than one memory init.");
			m_memoryInit = &_functionCall;
		}
	}
private:
	MemoryInitFinder() = default;
	FunctionCall* m_memoryInit = nullptr;
};

class FunctionArgumentCollector: ASTModifier
{
public:
	static std::set<YulString> run(Block& _block)
	{
		FunctionArgumentCollector functionArgumentCollector;
		functionArgumentCollector(_block);
		return functionArgumentCollector.m_functionArguments;
	}
	using ASTModifier::operator();
	void operator()(FunctionDefinition& _function) override
	{
		ASTModifier::operator()(_function);
		for (auto const& name:	_function.parameters)
			m_functionArguments.insert(name.name);
		for (auto const& name:	_function.returnVariables)
			m_functionArguments.insert(name.name);
	}
private:
	FunctionArgumentCollector() = default;
	set<YulString> m_functionArguments;
};

class VariableEscalator: ASTModifier
{
public:
	VariableEscalator(
		std::map<YulString, std::map<YulString, uint64_t>> const& _memorySlots,
		u256 const& _reservedMemory,
		NameDispenser& _nameDispenser
	): m_memorySlots(_memorySlots), m_reservedMemory(_reservedMemory), m_nameDispenser(_nameDispenser)
	{
	}

	using ASTModifier::operator();

	virtual void operator()(FunctionDefinition& _functionDefinition)
	{
		auto saved = m_currentFunctionMemorySlots;
		if (m_memorySlots.count(_functionDefinition.name))
		{
			m_currentFunctionMemorySlots = &m_memorySlots.at(_functionDefinition.name);
			for (auto const& param: _functionDefinition.parameters + _functionDefinition.returnVariables)
				if (m_currentFunctionMemorySlots->count(param.name))
				{
					// TODO: we cannot handle function parameters yet.
					m_currentFunctionMemorySlots = nullptr;
					break;
				}
		}
		else
			m_currentFunctionMemorySlots = nullptr;
		ASTModifier::operator()(_functionDefinition);
		m_currentFunctionMemorySlots = saved;
	}
	YulString getMemoryLocation(uint64_t _slot)
	{
		return YulString{util::toCompactHexWithPrefix(m_reservedMemory + 32 * _slot)};
	}
	static void appendMemoryStores(
		std::vector<Statement>& _statements,
		langutil::SourceLocation _loc,
		std::vector<std::pair<YulString, YulString>> const& _mstores
	)
	{
		for (auto const& [mpos, value]: _mstores)
			_statements.emplace_back(ExpressionStatement{_loc, FunctionCall{
				_loc,
				Identifier{_loc, "mstore"_yulstring},
				{
					Literal{_loc, LiteralKind::Number, mpos, {}},
					Identifier{_loc, value}
				}
			}});
	}
	virtual void operator()(Block& _block)
	{
		using OptionalStatements = std::optional<vector<Statement>>;
		if (!m_currentFunctionMemorySlots)
		{
			ASTModifier::operator()(_block);
			return;
		}
		auto containsVariableNeedingEscalation = [&](auto const& _variables) {
			return util::contains_if(_variables, [&](auto const& var) {
				return m_currentFunctionMemorySlots->count(var.name);
			});
		};

		util::iterateReplacing(
			_block.statements,
			[&](Statement& _statement)
			{
				auto defaultVisit = [&]() { ASTModifier::visit(_statement); return OptionalStatements{}; };
				return std::visit(util::GenericVisitor{
					[&](Assignment& _assignment) -> OptionalStatements
					{
						if (!containsVariableNeedingEscalation(_assignment.variableNames))
							return defaultVisit();
						visit(*_assignment.value);
						auto loc = _assignment.location;
						std::vector<Statement> result;
						if (_assignment.variableNames.size() == 1)
						{
							uint64_t slot = m_currentFunctionMemorySlots->at(_assignment.variableNames.front().name);
							result.emplace_back(ExpressionStatement{loc, FunctionCall{
								loc,
								Identifier{loc, "mstore"_yulstring},
								{
									Literal{loc, LiteralKind::Number, getMemoryLocation(slot), {}},
									std::move(*_assignment.value)
								}
							}});
						}
						else
						{
							std::vector<std::pair<YulString, YulString>> mstores;
							VariableDeclaration tempDecl{loc, {}, {}};
							for (auto& var: _assignment.variableNames)
								if (m_currentFunctionMemorySlots->count(var.name))
								{
									uint64_t slot = m_currentFunctionMemorySlots->at(var.name);
									var.name = m_nameDispenser.newName(var.name);
									mstores.emplace_back(getMemoryLocation(slot), var.name);
									tempDecl.variables.emplace_back(TypedName{loc, var.name, {}});
								}
							result.emplace_back(std::move(tempDecl));
							result.emplace_back(std::move(_assignment));
							appendMemoryStores(result, loc, mstores);
						}
						return {std::move(result)};
					},
					[&](VariableDeclaration& _varDecl) -> OptionalStatements
					{
						if (!containsVariableNeedingEscalation(_varDecl.variables))
							return defaultVisit();
						if (_varDecl.value)
							visit(*_varDecl.value);
						auto loc = _varDecl.location;
						std::vector<Statement> result;
						if (_varDecl.variables.size() == 1)
						{
							uint64_t slot = m_currentFunctionMemorySlots->at(_varDecl.variables.front().name);
							result.emplace_back(ExpressionStatement{loc, FunctionCall{
								loc,
								Identifier{loc, "mstore"_yulstring},
								{
									Literal{loc, LiteralKind::Number, getMemoryLocation(slot), {}},
									_varDecl.value ? std::move(*_varDecl.value) : Literal{loc, LiteralKind::Number, "0"_yulstring, {}}
								}
							}});
						}
						else
						{
							std::vector<std::pair<YulString, YulString>> mstores;
							for (auto& var: _varDecl.variables)
								if (m_currentFunctionMemorySlots->count(var.name))
								{
									uint64_t slot = m_currentFunctionMemorySlots->at(var.name);
									var.name = m_nameDispenser.newName(var.name);
									mstores.emplace_back(getMemoryLocation(slot), var.name);
								}
							result.emplace_back(std::move(_varDecl));
							appendMemoryStores(result, loc, mstores);
						}
						return {std::move(result)};
					},
					[&](auto&) { return defaultVisit(); }
				}, _statement);
			});
	}
	virtual void visit(Expression& _expression)
	{
		if (
			auto identifier = std::get_if<Identifier>(&_expression);
			identifier && m_currentFunctionMemorySlots && m_currentFunctionMemorySlots->count(identifier->name)
		)
		{
			auto loc = identifier->location;
			_expression = FunctionCall {
				loc,
				Identifier{loc, "mload"_yulstring}, {
					Literal {
						loc,
						LiteralKind::Number,
						getMemoryLocation(m_currentFunctionMemorySlots->at(identifier->name)),
						{}
					}
				}
			};
		}
		else
			ASTModifier::visit(_expression);
	}
private:
	std::map<YulString, std::map<YulString, uint64_t>> const& m_memorySlots;
	u256 m_reservedMemory;
	NameDispenser& m_nameDispenser;
	std::map<YulString, uint64_t> const* m_currentFunctionMemorySlots = nullptr;
};

}

void MemoryEscalator::runUntilStabilized(OptimiserStepContext& _context, Object& _object, bool _optimizeStackAllocation)
{
	uint64_t const maxRuns = 128;
	uint64_t runs = 0;
	while (run(_context, _object, _optimizeStackAllocation) && runs < maxRuns)
		++runs;
}

bool MemoryEscalator::run(OptimiserStepContext& _context, Object& _object, bool _optimizeStackAllocation)
{
	if (!_object.code)
		return false;

	auto functionStackErrorInfo = CompilabilityChecker::run(_context.dialect, _object, _optimizeStackAllocation);
	if (functionStackErrorInfo.empty())
		return false;

	yulAssert(dynamic_cast<EVMDialect const*>(&_context.dialect), "MemoryEscalator can only be run on EVMDialect objects.");
	Literal* memoryInitLiteral = nullptr;
	if (FunctionCall* memoryInit = MemoryInitFinder::run(*_object.code))
		memoryInitLiteral = std::get_if<Literal>(&memoryInit->arguments.back());
	if (!memoryInitLiteral)
		return false;
	u256 reservedMemory = valueOfLiteral(*memoryInitLiteral);

	auto callGraph = CallGraphGenerator::callGraph(*_object.code).functionCalls;

	std::set<YulString> containedInCycle;
	auto findCycles = [
		&,
		visited = std::map<YulString, uint64_t>{},
		currentPath = std::vector<YulString>{}
	](YulString _node, auto& _recurse) mutable -> void
	{
		if (auto it = std::find(currentPath.begin(), currentPath.end(), _node); it != currentPath.end())
			containedInCycle.insert(it, currentPath.end());
		else
		{
			visited[_node] = currentPath.size();
			currentPath.emplace_back(_node);
			for (auto const& child: callGraph[_node])
				_recurse(child, _recurse);
			currentPath.pop_back();
		}
	};
	findCycles(YulString{}, findCycles);

	using SlotAllocations = map<YulString, map<YulString, size_t>>;
	SlotAllocations slotAllocations;
	uint64_t requiredSlots = 0;

	auto allocateMemorySlots = [
		&,
		visited = std::set<YulString>{},
		numUsedSlots = uint64_t(0),
		functionArguments = FunctionArgumentCollector::run(*_object.code)
	](YulString _function, auto& _recurse) mutable -> bool {
		if (visited.count(_function))
			return true;
		visited.insert(_function);

		uint64_t previouslyUsedSlots = numUsedSlots;
		if (functionStackErrorInfo.count(_function))
		{
			if (containedInCycle.count(_function))
				return false;
			auto const& stackErrorInfo = functionStackErrorInfo.at(_function);
			for (auto const& variable: stackErrorInfo.variables)
				// TODO: deal with function arguments.
				if (!functionArguments.count(variable) && !variable.empty())
					slotAllocations[_function][variable] = numUsedSlots++;
		}
		requiredSlots = std::max(requiredSlots, numUsedSlots);
		for (auto const& child: callGraph[_function])
			if (!_recurse(child, _recurse))
				return false;
		numUsedSlots = previouslyUsedSlots;
		return true;
	};
	if (!allocateMemorySlots(YulString{}, allocateMemorySlots) || !requiredSlots)
		return false;

	memoryInitLiteral->value = YulString{util::toCompactHexWithPrefix(reservedMemory + 32 * requiredSlots)};

	if (_object.code)
		VariableEscalator{slotAllocations, reservedMemory, _context.dispenser}(*_object.code);

	return true;
}