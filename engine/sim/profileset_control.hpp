#pragma once

#include "player/player.hpp"
#include "player/rating.hpp"
#include "sc_enums.hpp"
#include "util/generic.hpp"

#include <atomic>
#include <functional>
#include <mutex>

struct sim_t;

enum call_point_e
{
  CALL_POINT_NONE,
  POST_INIT,
  POST_ITER
};

template <typename T>
struct data_wrapper_t
{
private:
  std::scoped_lock<std::recursive_mutex> lock;

public:
  const T& data;

  data_wrapper_t( const T& data, std::recursive_mutex& m ) : lock( m ), data( data )
  {
  }
};

struct exit_reason_t
{
  const std::string profileset_name;
  const call_point_e exit_point;
  const std::string exit_reason;
};

struct profileset_controller_data_t : private noncopyable
{
  const std::string key;
  std::string_view options;
  std::vector<exit_reason_t> exit_reasons;

  profileset_controller_data_t( std::string_view, std::string_view );
  virtual ~profileset_controller_data_t() = default;

  virtual void report_html_options( std::ostream& ) const;
  virtual void report_html_profileset( std::ostream& ) const;
  virtual void report_json_options( js::JsonOutput& ) const;
  virtual void report_json_profileset( js::JsonOutput& ) const;
};

struct profileset_controller_data_wrapper_t : private noncopyable
{
private:
  static std::atomic_uint id_generator;

public:
  std::recursive_mutex mutex;

  const unsigned int id;
  const std::string key;
  std::string_view options;
  std::unique_ptr<profileset_controller_data_t> data;

  profileset_controller_data_wrapper_t( std::string, std::string_view );

  void construct_controller( sim_t* );
};

struct profileset_controller_t : private noncopyable
{
  using controller_factory_t = std::function<std::unique_ptr<profileset_controller_t>( sim_t*, unsigned int )>;
  using data_factory_t =
      std::function<std::unique_ptr<profileset_controller_data_t>( std::string_view, std::string_view )>;
  using factory_fn_pair_t = std::pair<controller_factory_t, data_factory_t>;

protected:
  friend profileset_controller_data_wrapper_t;
  static std::unordered_map<std::string, factory_fn_pair_t> factory;

public:
  static bool register_controller( std::string, factory_fn_pair_t&& );
  static bool controller_exists( std::string );

  using data_t = profileset_controller_data_t;
  static void evaluate( sim_t* sim, call_point_e call_point );

  sim_t* parent;
  sim_t* sim;
  const unsigned int id;
  std::vector<std::unique_ptr<option_t>> options;

  profileset_controller_t( sim_t*, unsigned int );
  virtual ~profileset_controller_t() = default;

  const std::string message( call_point_e );
  void add_option( std::unique_ptr<option_t>&& );

  virtual const std::string name() const   = 0;
  virtual const std::string reason() const = 0;
  virtual void create_options()
  {
  }
  virtual bool evaluate_post_init()
  {
    return true;
  }
  virtual bool evaluate_post_iter()
  {
    return true;
  }

protected:
  template <typename T>
  data_wrapper_t<T> get_data();
  template <typename T>
  void set_data( T&& data );
  void set_exit_reason( exit_reason_t&& );
};

namespace profileset_controller
{
const std::string call_point_string( call_point_e call_point );
void report_html( const sim_t&, std::ostream& );
void report_json( const sim_t&, js::JsonOutput& output );

template <typename T>
profileset_controller_t::factory_fn_pair_t create_fn_pair()
{
  return { []( sim_t* sim, unsigned int id ) { return std::make_unique<T>( sim, id ); },
           []( std::string_view key, std::string_view options ) {
             return std::make_unique<typename T::data_t>( key, options );
           } };
}
};  // namespace profileset_controller

struct set_bonus_enabled_t : profileset_controller_t
{
  using data_t = profileset_controller_data_t;

  player_t* target_player;
  set_bonus_type_e tier;
  set_bonus_e count;

  set_bonus_enabled_t( sim_t* sim, unsigned int id ) : profileset_controller_t( sim, id )
  {
  }

  const std::string name() const override
  {
    return "set_bonus_enabled";
  }
  bool evaluate_post_init() override;
  const std::string reason() const override;
  void create_options() override;
};
