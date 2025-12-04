#pragma once

#include "player/player.hpp"
#include "player/rating.hpp"
#include "sc_enums.hpp"
#include "util/generic.hpp"

#include <functional>
#include <mutex>
#include <atomic>

struct sim_t;

template <typename T>
struct data_wrapper_t
{
private:
  std::scoped_lock<std::recursive_mutex> lock;
public:
  const T& data;

  data_wrapper_t( const T& data, std::recursive_mutex& m ) : lock( m ), data( data ) {}
};

enum call_point_e
{
  CALL_POINT_NONE,
  POST_INIT,
  POST_ITER
};

struct exit_reason_t
{
  const std::string profileset_name;
  const call_point_e exit_point;
  const std::string exit_reason;
};

struct profileset_controller_data_t : private noncopyable
{
  std::vector<exit_reason_t> exit_reasons;
  std::vector<std::unique_ptr<option_t>> options;

  profileset_controller_data_t( std::string_view );
  virtual ~profileset_controller_data_t() = default;

  virtual void report_html_options( std::ostream& ) {}
  virtual void report_html_profileset( std::ostream& ) {}
};

struct profileset_controller_data_wrapper_t : private noncopyable
{
  static std::atomic_uint id_generator;
  std::recursive_mutex mutex;
  unsigned int id;
  std::string key;
  std::unique_ptr<profileset_controller_data_t> data;

  profileset_controller_data_wrapper_t( std::string, std::string_view );

  void construct_controller( sim_t* );
};

struct profileset_controller_t : private noncopyable
{
  using controller_factory_t = std::function<std::unique_ptr<profileset_controller_t>(sim_t*, unsigned int)>;
  using data_factory_t = std::function<std::unique_ptr<profileset_controller_data_t>(std::string_view)>;
  using factory_fn_pair_t = std::pair<controller_factory_t, data_factory_t>;
protected:
  friend profileset_controller_data_wrapper_t;
  static std::unordered_map<std::string, factory_fn_pair_t> factory;
public:
  static bool register_controller( std::string, factory_fn_pair_t&& );
  static bool controller_exists( std::string );

  using data_t = profileset_controller_data_t;
  static const std::string call_point_string( call_point_e call_point );
  static void evaluate( sim_t* sim, call_point_e call_point );

  static void html_report( const sim_t&, std::ostream& );

  sim_t* parent;
  sim_t* sim;
  const unsigned int id;

  profileset_controller_t( sim_t*, unsigned int );
  virtual ~profileset_controller_t() = default;

  const std::string message( call_point_e );
  // void add_option( std::unique_ptr<option_t>&& );

  virtual const std::string name() const   = 0;
  virtual const std::string reason() const = 0;
  // virtual void create_options() {}
  virtual bool evaluate_post_init() {
    return true;
  }
  virtual bool evaluate_post_iter() {
    return true;
  }

protected:
  template <typename T>
  data_wrapper_t<T> get_data();
  template <typename T>
  void set_data( T&& data );
  void set_exit_reason( exit_reason_t&& );
};

struct min_player_stat_t : profileset_controller_t
{
  /*
   * This sim controller doesn't work, as at all controller evaluation points
   * only have base rating provided by the class/spec. If gear stats were to
   * be set once on actor init and preserved between iterations, this would be
   * fixed.
   */
  using data_t = profileset_controller_data_t;

  player_t* target_player;
  stat_e rating;
  double min_rating;

  const std::string name() const override
  {
    return "min_player_stat";
  }
  bool evaluate_post_init() override;
  const std::string reason() const override;
};

struct tier_set_count_t : profileset_controller_t
{
  using data_t = profileset_controller_data_t;

  player_t* target_player;
  set_bonus_type_e tier;
  set_bonus_e count;
  int test;

  tier_set_count_t( sim_t* sim, unsigned int id ) : profileset_controller_t( sim, id )
  {
  }
  const std::string name() const override
  {
    return "tier_set_count";
  }
  bool evaluate_post_init() override;
  const std::string reason() const override;
  // void create_options() override;
};
