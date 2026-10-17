// Neural network policy description: inputs (sim expressions + normalization),
// actions (APL action lines), and a stack of dense layers.
//
// File format (line-oriented; blank lines and lines starting with '#' are ignored):
//
//   version 1
//   inputs <N>
//   <expression> <offset> <scale>         (N lines)
//   actions <M>
//   <action line>                         (M lines; full APL action string)
//   layer <n_in> <n_out> <activation>     (activation: linear | relu | tanh)
//   <n_in numbers>                        (n_out lines; row i = weights of output i)
//   <n_out numbers>                       (biases)
//   layer ...                             (repeated until end of file)
//
// The network computes, for each layer, y = act(W x + b), where the first
// layer's input is the normalized feature vector (expr - offset) / scale and the
// last layer's output is one score (logit) per action.

#pragma once

#include <cstddef>
#include <istream>
#include <string>
#include <vector>

constexpr double RAW_LIMIT  = 1e6;   // keeps "infinite" expressions (e.g. time to die) finite
constexpr double NORM_LIMIT = 10.0;  // clamp for normalized features

enum class nn_activation_e
{
  LINEAR,
  RELU,
  TANH
};

struct nn_input_t
{
  std::string expression;  // unparsed sim expression, bound to the actor later
  double offset = 0.0;
  double scale  = 1.0;     // nonzero; feature = (value - offset) / scale
};

struct nn_layer_t
{
  std::size_t n_in  = 0;
  std::size_t n_out = 0;
  nn_activation_e activation = nn_activation_e::LINEAR;
  std::vector<double> weights;  // row-major, n_out x n_in
  std::vector<double> bias;     // n_out

  double weight( std::size_t out, std::size_t in ) const
  { return weights[ out * n_in + in ]; }
};

struct nn_policy_data_t
{
  int version = 0;
  std::vector<nn_input_t> inputs;
  std::vector<std::string> actions;  // unparsed APL action lines, one per output
  std::vector<nn_layer_t> layers;

  std::size_t input_size()  const { return inputs.size(); }
  std::size_t output_size() const { return actions.size(); }
};

nn_policy_data_t read_nn_policy( const std::string& path );
