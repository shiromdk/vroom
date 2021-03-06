/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "problems/cvrp/local_search/reverse_2_opt.h"

cvrp_reverse_two_opt::cvrp_reverse_two_opt(const input& input,
                                           const solution_state& sol_state,
                                           std::vector<index_t>& s_route,
                                           index_t s_vehicle,
                                           index_t s_rank,
                                           std::vector<index_t>& t_route,
                                           index_t t_vehicle,
                                           index_t t_rank)
  : ls_operator(input,
                sol_state,
                s_route,
                s_vehicle,
                s_rank,
                t_route,
                t_vehicle,
                t_rank) {
  assert(s_vehicle != t_vehicle);
  assert(s_route.size() >= 1);
  assert(t_route.size() >= 1);
  assert(s_rank < s_route.size());
  assert(t_rank < t_route.size());
}

void cvrp_reverse_two_opt::compute_gain() {
  const auto& m = _input.get_matrix();
  const auto& v_source = _input._vehicles[s_vehicle];
  const auto& v_target = _input._vehicles[t_vehicle];

  index_t s_index = _input._jobs[s_route[s_rank]].index();
  index_t t_index = _input._jobs[t_route[t_rank]].index();
  index_t last_s = _input._jobs[s_route.back()].index();
  index_t first_t = _input._jobs[t_route.front()].index();
  stored_gain = 0;
  bool last_in_source = (s_rank == s_route.size() - 1);
  bool last_in_target = (t_rank == t_route.size() - 1);

  // Cost of swapping route for vehicle s_vehicle after step
  // s_rank with route for vehicle t_vehicle up to step
  // t_rank, but reversed.

  // Add new source -> target edge.
  stored_gain -= m[s_index][t_index];

  // Cost of reversing target route portion.
  stored_gain += _sol_state.fwd_costs[t_vehicle][t_rank];
  stored_gain -= _sol_state.bwd_costs[t_vehicle][t_rank];

  if (!last_in_target) {
    // Spare next edge in target route.
    index_t next_index = _input._jobs[t_route[t_rank + 1]].index();
    stored_gain += m[t_index][next_index];
  }

  if (!last_in_source) {
    // Spare next edge in source route.
    index_t next_index = _input._jobs[s_route[s_rank + 1]].index();
    stored_gain += m[s_index][next_index];

    // Part of source route is moved to target route.
    index_t next_s_index = _input._jobs[s_route[s_rank + 1]].index();

    // Cost or reverting source route portion.
    stored_gain += _sol_state.fwd_costs[s_vehicle].back();
    stored_gain -= _sol_state.fwd_costs[s_vehicle][s_rank + 1];
    stored_gain -= _sol_state.bwd_costs[s_vehicle].back();
    stored_gain += _sol_state.bwd_costs[s_vehicle][s_rank + 1];

    if (last_in_target) {
      if (v_target.has_end()) {
        // Handle target route new end.
        auto end_t = v_target.end.get().index();
        stored_gain += m[t_index][end_t];
        stored_gain -= m[next_s_index][end_t];
      }
    } else {
      // Add new target -> source edge.
      index_t next_t_index = _input._jobs[t_route[t_rank + 1]].index();
      stored_gain -= m[next_s_index][next_t_index];
    }
  }

  if (v_source.has_end()) {
    // Update cost to source end because last job changed.
    auto end_s = v_source.end.get().index();
    stored_gain += m[last_s][end_s];
    stored_gain -= m[first_t][end_s];
  }

  if (v_target.has_start()) {
    // Spare cost from target start because first job changed.
    auto start_t = v_target.start.get().index();
    stored_gain += m[start_t][first_t];
    if (!last_in_source) {
      stored_gain -= m[start_t][last_s];
    } else {
      // No job from source route actually swapped to target route.
      if (!last_in_target) {
        // Going straight from start to next job in target route.
        index_t next_index = _input._jobs[t_route[t_rank + 1]].index();
        stored_gain -= m[start_t][next_index];
      } else {
        // Emptying the whole target route here, so also gaining cost
        // to end if it exists.
        if (v_target.has_end()) {
          auto end_t = v_target.end.get().index();
          stored_gain += m[t_index][end_t];
        }
      }
    }
  }

  gain_computed = true;
}

bool cvrp_reverse_two_opt::is_valid() {
  bool valid = (_sol_state.bwd_skill_rank[s_vehicle][t_vehicle] <= s_rank + 1);

  valid &= (t_rank < _sol_state.fwd_skill_rank[t_vehicle][s_vehicle]);

  valid &= (_sol_state.fwd_amounts[s_vehicle][s_rank] +
              _sol_state.fwd_amounts[t_vehicle][t_rank] <=
            _input._vehicles[s_vehicle].capacity);
  valid &= (_sol_state.bwd_amounts[t_vehicle][t_rank] +
              _sol_state.bwd_amounts[s_vehicle][s_rank] <=
            _input._vehicles[t_vehicle].capacity);

  return valid;
}

void cvrp_reverse_two_opt::apply() {
  auto nb_source = s_route.size() - 1 - s_rank;

  t_route.insert(t_route.begin(),
                 s_route.rbegin(),
                 s_route.rbegin() + nb_source);
  s_route.erase(s_route.begin() + s_rank + 1, s_route.end());
  s_route.insert(s_route.end(),
                 t_route.rend() - t_rank - nb_source - 1,
                 t_route.rend() - nb_source);
  t_route.erase(t_route.begin() + nb_source,
                t_route.begin() + nb_source + t_rank + 1);
}

std::vector<index_t> cvrp_reverse_two_opt::addition_candidates() const {
  return {s_vehicle, t_vehicle};
}

std::vector<index_t> cvrp_reverse_two_opt::update_candidates() const {
  return {s_vehicle, t_vehicle};
}
