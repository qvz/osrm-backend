#ifndef OSRM_ENGINE_GUIDANCE_COLLAPSE_HPP

#include "engine/guidance/route_step.hpp"
#include "util/attributes.hpp"

#include <type_traits>
#include <vector>

namespace osrm
{
namespace engine
{
namespace guidance
{

// Multiple possible reasons can result in unnecessary/confusing instructions
// A prime example would be a segregated intersection. Turning around at this
// intersection would result in two instructions to turn left.
// Collapsing such turns into a single turn instruction, we give a clearer
// set of instructionst that is not cluttered by unnecessary turns/name changes.
OSRM_ATTR_WARN_UNUSED
std::vector<RouteStep> CollapseTurnInstructions(std::vector<RouteStep> steps);

// A combined turn is a set of two instructions that actually form a single turn, as far as we
// perceive it. A u-turn consisting of two left turns is one such example. But there are also lots
// of other items that influence how we combine turns. This function is an entry point, defining the
// possibility to select one of multiple strategies when combining a turn with another one.
template <typename CombinedTurnStrategy, typename SignageStrategy>
RouteStep CombineRouteSteps(const RouteStep &step_at_turn_location,
                            const RouteStep &step_after_turn_location,
                            const CombinedTurnStrategy combined_turn_stragey,
                            const SignageStrategy signage_strategy);

// TAGS
// These tags are used to ensure correct strategy usage. Make sure your new strategy is derived from
// (at least) one of these tags. It can only be used for the intended tags, to ensure we don't
// accidently use a lane strategy to cover signage
struct CombineStrategy
{
};

struct SignageStrategy
{
};

struct LaneStrategy
{
};

// Return the step at the turn location, without modification
struct NoModificationStrategy : CombineStrategy, SignageStrategy, LaneStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// transfer the turn type from the second step
struct TransferTurnTypeStrategy : CombineStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

struct AdjustToCombinedTurnAngleStrategy : CombineStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

struct SetFixedInstructionStrategy : CombineStrategy
{
    SetFixedInstructionStrategy(const extractor::guidance::TurnInstruction instruction);
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const extractor::guidance::TurnInstruction instruction;
};

struct StaggeredTurnStrategy : CombineStrategy
{
    StaggeredTurnStrategy(const RouteStep &step_prior_to_intersection);

    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;

    const RouteStep &step_prior_to_intersection;
};

// Signage Strategies
struct TransferSignageStrategy : SignageStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// Lane Strategies
struct TransferLanesStrategy : LaneStrategy
{
    void operator()(RouteStep &step_at_turn_location, const RouteStep &transfer_from_step) const;
};

// IMPLEMENTATIONS
template <typename CombineStrategyClass, typename SignageStrategyClass, typename LaneStrategyClass>
void CombineRouteSteps(RouteStep &step_at_turn_location,
                       RouteStep &step_after_turn_location,
                       CombineStrategyClass combined_turn_stragey,
                       SignageStrategyClass signage_strategy,
                       LaneStrategyClass lane_strategy)
{
    // assign the combined turn type
    static_assert(std::is_base_of<CombineStrategy, CombineStrategyClass>::value,
                  "Supplied Strategy isn't a combine strategy.");
    combined_turn_stragey(step_at_turn_location, step_after_turn_location);

    // assign the combind signage
    static_assert(std::is_base_of<LaneStrategy, LaneStrategyClass>::value,
                  "Supplied Strategy isn't a signage strategy.");
    signage_strategy(step_at_turn_location, step_after_turn_location);

    // assign the desired turn lanes
    static_assert(std::is_base_of<LaneStrategy, LaneStrategyClass>::value,
                  "Supplied Strategy isn't a lane strategy.");
    lane_strategy(step_at_turn_location, step_after_turn_location);

    // further stuff should happen here as well
    step_at_turn_location.ElongateBy(step_after_turn_location);
    step_after_turn_location.Invalidate();
}

} /* namespace guidance */
} /* namespace osrm */
} /* namespace osrm */

#endif /* OSRM_ENGINE_GUIDANCE_COLLAPSE_HPP_ */
