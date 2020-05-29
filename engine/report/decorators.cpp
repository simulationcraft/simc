// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "decorators.hpp"

#include "action/sc_action.hpp"
#include "buff/sc_buff.hpp"
#include "dbc/dbc.hpp"
#include "item/item.hpp"
#include "player/sc_player.hpp"
#include "util/util.hpp"
#include "report/color.hpp"
#include <vector>

namespace {

  const color::rgb& item_quality_color(const item_t& item)
  {
    switch (item.parsed.data.quality)
    {
    case 1:
      return color::COMMON;
    case 2:
      return color::UNCOMMON;
    case 3:
      return color::RARE;
    case 4:
      return color::EPIC;
    case 5:
      return color::LEGENDARY;
    case 6:
      return color::HEIRLOOM;
    case 7:
      return color::HEIRLOOM;
    default:
      return color::POOR;
    }
  }

  template <typename T>
  class html_decorator_t
  {
    std::vector<std::string> m_parameters;

  protected:
    bool find_affixes(std::string& prefix, std::string& suffix) const
    {
      std::string tokenized_name = url_name();
      util::tokenize(tokenized_name);
      std::string obj_token = token();

      std::string::size_type affix_offset = obj_token.find(tokenized_name);

      // Add an affix to the name, if the name does not match the
      // spell name. Affix is either the prefix- or suffix portion of the
      // non matching parts of the stats name.
      if (affix_offset != std::string::npos && tokenized_name != obj_token)
      {
        // Suffix
        if (affix_offset == 0)
          suffix = obj_token.substr(tokenized_name.size());
        // Prefix
        else if (affix_offset > 0)
          prefix = obj_token.substr(0, affix_offset);
      }
      else if (affix_offset == std::string::npos)
        suffix = obj_token;

      return !prefix.empty() || !suffix.empty();
    }

  public:
    html_decorator_t()
    { }

    virtual ~html_decorator_t() = default;

    virtual std::string url_params() const
    {
      std::stringstream s;

      // Add url parameters
      auto parameters = this->parms();
      if (parameters.size() > 0)
      {
        s << "?";

        for (size_t i = 0, end = parameters.size(); i < end; ++i)
        {
          s << parameters[i];

          if (i < end - 1)
          {
            s << "&";
          }
        }
      }

      return s.str();
    }

    T& add_parm(const std::string& n, const std::string& v)
    {
      m_parameters.push_back(n + "=" + v);
      return *this;
    }

    T& add_parm(const std::string& n, const char* v)
    {
      return add_parm(n, std::string(v));
    }

    template <typename P_VALUE>
    T& add_parm(const std::string& n, P_VALUE& v)
    {
      m_parameters.push_back(n + "=" + util::to_string(v));
      return *this;
    }

    virtual std::vector<std::string> parms() const
    {
      return m_parameters;
    }

    virtual std::string base_url() const = 0;

    virtual bool can_decorate() const = 0;

    // Full blown name, displayed as the link name in the HTML report
    virtual std::string url_name() const = 0;

    // Tokenized name of the spell
    virtual std::string token() const = 0;

    // An optional prefix applied to url_name (included in the link)
    virtual std::string url_name_prefix() const
    {
      return std::string();
    }

    virtual std::string decorate() const
    {
      if (!can_decorate())
      {
        return "<a href=\"#\">" + token() + "</a>";
      }

      std::stringstream s;

      std::string prefix, suffix;
      find_affixes(prefix, suffix);

      if (!prefix.empty())
      {
        s << "(" << prefix << ")&#160;";
      }

      // Generate base url
      s << base_url();
      s << url_params();

      // Close url
      s << "\">";

      // Add optional prefix to the url name
      s << url_name_prefix();

      // Add name of the spell
      s << url_name();

      // Close <a>
      s << "</a>";

      // Add suffix if present
      if (!suffix.empty())
      {
        s << "&#160;(" << suffix << ")";
      }

      return s.str();
    }
  };

  template <typename T>
  class spell_decorator_t : public html_decorator_t<spell_decorator_t<T>>
  {
    using super = html_decorator_t<spell_decorator_t<T>>;
  protected:
    const T* m_obj;

  public:
    spell_decorator_t(const T* obj) :
      super(), m_obj(obj)
    { }

    std::string base_url() const override
    {
      std::stringstream s;

      s << "<a href=\"https://" << report_decorators::decoration_domain(*this->m_obj->sim)
        << ".wowhead.com/spell=" << this->m_obj->data_reporting().id();

      return s.str();
    }

    bool can_decorate() const override
    {
      return this->m_obj->sim->decorated_tooltips &&
        this->m_obj->data_reporting().id() > 0;
    }

    std::string url_name() const override
    {
      return util::encode_html(m_obj->data_reporting().id() ? m_obj->data_reporting().name_cstr() : m_obj->name());
    }

    std::string token() const override
    {
      return util::encode_html(m_obj->name());
    }

    std::vector<std::string> parms() const override
    {
      auto parameters = super::parms();

      if (m_obj->item)
      {
        parameters.push_back("ilvl=" + util::to_string(m_obj->item->item_level()));
      }

      return parameters;
    }
  };

  // Generic spell data decorator, supports player and item driven spell data
  class spell_data_decorator_t : public html_decorator_t<spell_data_decorator_t>
  {
    const sim_t* m_sim;
    const spell_data_t* m_spell;
    const item_t* m_item;

  public:
    spell_data_decorator_t(const sim_t* obj, const spell_data_t* spell) :
      html_decorator_t(), m_sim(obj), m_spell(spell),
      m_item(nullptr)
    { }

    spell_data_decorator_t(const sim_t* obj, const spell_data_t& spell) :
      spell_data_decorator_t(obj, &spell)
    { }

    spell_data_decorator_t(const player_t* obj, const spell_data_t* spell)
      : html_decorator_t(), m_sim(obj->sim), m_spell(spell), m_item(nullptr)
    {
    }

    spell_data_decorator_t(const player_t* obj, const spell_data_t& spell) :
      spell_data_decorator_t(obj, &spell)
    { }

    std::vector<std::string> parms() const override
    {
      auto params = html_decorator_t::parms();

      // if ( m_player && m_player -> specialization() != SPEC_NONE )
      //{
      //  params.push_back( "spec=" + util::to_string( m_player -> specialization() ) );
      //}

      if (m_item)
      {
        params.push_back("ilvl=" + util::to_string(m_item->item_level()));
      }

      return params;
    }
    std::string base_url() const override
    {
      std::stringstream s;

      s << "<a href=\"https://" << report_decorators::decoration_domain(*m_sim) << ".wowhead.com/spell=" << m_spell->id();

      return s.str();
    }

    spell_data_decorator_t& item(const item_t* obj)
    {
      m_item = obj; return *this;
    }

    spell_data_decorator_t& item(const item_t& obj)
    {
      m_item = &obj; return *this;
    }

    bool can_decorate() const override
    {
      return m_sim->decorated_tooltips && m_spell->id() > 0;
    }
    std::string url_name() const override
    {
      return util::encode_html(m_spell->name_cstr());
    }
    std::string token() const override
    {
      std::string token = m_spell->name_cstr();
      util::tokenize(token);
      return util::encode_html(token);
    }
  };

  class item_decorator_t : public html_decorator_t<item_decorator_t>
  {
    const item_t* m_item;

  public:
    item_decorator_t(const item_t* obj) :
      html_decorator_t(), m_item(obj)
    { }

    item_decorator_t(const item_t& obj) :
      item_decorator_t(&obj)
    { }

    std::vector<std::string> parms() const override
    {
      auto params = html_decorator_t::parms();

      if (m_item->parsed.enchant_id > 0)
      {
        params.push_back("ench=" + util::to_string(m_item->parsed.enchant_id));
      }

      std::stringstream gem_str;
      for (size_t i = 0, end = m_item->parsed.gem_id.size(); i < end; ++i)
      {
        if (m_item->parsed.gem_id[i] == 0)
        {
          continue;
        }

        if (gem_str.tellp() > 0)
        {
          gem_str << ":";
        }

        gem_str << util::to_string(m_item->parsed.gem_id[i]);

        // Include relic bonus ids
        if (i < m_item->parsed.gem_bonus_id.size())
        {
          range::for_each(m_item->parsed.gem_bonus_id[i],
            [&gem_str](unsigned bonus_id) { gem_str << ":" << bonus_id; });
        }
      }

      if (gem_str.tellp() > 0)
      {
        params.push_back("gems=" + gem_str.str());
      }

      std::stringstream bonus_str;
      for (size_t i = 0, end = m_item->parsed.bonus_id.size(); i < end; ++i)
      {
        bonus_str << util::to_string(m_item->parsed.bonus_id[i]);

        if (i < end - 1)
        {
          bonus_str << ":";
        }
      }

      if (bonus_str.tellp() > 0)
      {
        params.push_back("bonus=" + bonus_str.str());
      }

      std::stringstream azerite_str;
      if (!m_item->parsed.azerite_ids.empty())
      {
        azerite_str << util::class_id(m_item->player->type) << ":";
      }
      for (size_t i = 0, end = m_item->parsed.azerite_ids.size(); i < end; ++i)
      {
        azerite_str << util::to_string(m_item->parsed.azerite_ids[i]);

        if (i < end - 1)
        {
          azerite_str << ":";
        }
      }

      if (azerite_str.tellp() > 0)
      {
        params.push_back("azerite-powers=" + azerite_str.str());
      }

      params.push_back("ilvl=" + util::to_string(m_item->item_level()));

      return params;
    }
    std::string base_url() const override
    {
      std::stringstream s;

      s << "<a "
        << "style=\"color:" << item_quality_color(*m_item) << ";\" "
        << "href=\"https://" << report_decorators::decoration_domain(*m_item->sim) << ".wowhead.com/item=" << m_item->parsed.data.id;

      return s.str();
    }

    bool can_decorate() const override
    {
      return m_item->sim->decorated_tooltips && m_item->parsed.data.id > 0;
    }
    std::string url_name() const override
    {
      return util::encode_html(m_item->full_name());
    }
    std::string token() const override
    {
      return util::encode_html(m_item->name());
    }
  };

  class buff_decorator_t : public spell_decorator_t<buff_t>
  {
    using super = spell_decorator_t<buff_t>;

  protected:
    std::vector<std::string> parms() const override
    {
      std::vector<std::string> parms = super::parms();

      // if ( m_obj -> source && m_obj -> source -> specialization() != SPEC_NONE )
      //{
      //  parms.push_back( "spec=" + util::to_string( m_obj -> source -> specialization() ) );
      //}

      return parms;
    }

  public:
    buff_decorator_t(const buff_t* obj) :
      super(obj)
    { }

    buff_decorator_t(const buff_t& obj) :
      buff_decorator_t(&obj)
    { }

    // Buffs have pet names included in them
    std::string url_name_prefix() const override
    {
      if (m_obj->source && m_obj->source->is_pet())
      {
        return util::encode_html(m_obj->source->name_str) + ":&#160;";
      }

      return std::string();
    }
  };

  class action_decorator_t : public spell_decorator_t<action_t>
  {
    using super = spell_decorator_t<action_t>;

  protected:
    std::vector<std::string> parms() const override
    {
      std::vector<std::string> parms = super::parms();

      // if ( m_obj -> player && m_obj -> player -> specialization() != SPEC_NONE )
      //{
      //  parms.push_back( "spec=" + util::to_string( m_obj -> player -> specialization() ) );
      //}

      return parms;
    }

  public:
    action_decorator_t(const action_t* obj) :
      super(obj)
    { }

    action_decorator_t(const action_t& obj) :
      action_decorator_t(&obj)
    { }
  };


  } // unnamed namespace

namespace report_decorators {

  std::string decoration_domain(const sim_t& sim)
  {
#if SC_BETA == 0
    if (maybe_ptr(sim.dbc->ptr))
    {
      return "ptr";
    }
    else
    {
      return "www";
    }
#else
    return "beta";
    (void)sim;
#endif
  }

  std::string decorated_spell_name(const sim_t& sim, const spell_data_t& spell, const std::string& parms_str)
  {
    std::stringstream s;

    if (sim.decorated_tooltips == false)
    {
      s << "<a href=\"#\">" << util::encode_html(spell.name_cstr()) << "</a>";
    }
    else
    {
      s << "<a href=\"https://" << decoration_domain(sim) << ".wowhead.com/spell=" << spell.id()
        << (!parms_str.empty() ? "?" + parms_str : "") << "\">" << util::encode_html(spell.name_cstr()) << "</a>";
    }

    return s.str();
  }

  std::string decorated_item_name(const item_t* item)
  {
    std::stringstream s;

    if (item->sim->decorated_tooltips == false || item->parsed.data.id == 0)
    {
      s << "<a style=\"color:" << item_quality_color(*item) << ";\" href=\"#\">" << util::encode_html(item->full_name()) << "</a>";
    }
    else
    {
      std::vector<std::string> params;
      if (item->parsed.enchant_id > 0)
      {
        params.push_back("enchantment=" + util::to_string(item->parsed.enchant_id));
      }

      std::string gem_str = "";
      for (size_t i = 0; i < item->parsed.gem_id.size(); ++i)
      {
        if (item->parsed.gem_id[i] == 0)
        {
          continue;
        }

        if (!gem_str.empty())
        {
          gem_str += ",";
        }

        gem_str += util::to_string(item->parsed.gem_id[i]);
      }

      if (!gem_str.empty())
      {
        params.push_back("gems=" + gem_str);
      }

      std::string bonus_str = "";
      for (size_t i = 0; i < item->parsed.bonus_id.size(); ++i)
      {
        bonus_str += util::to_string(item->parsed.bonus_id[i]);
        if (i < item->parsed.bonus_id.size() - 1)
        {
          bonus_str += ",";
        }
      }

      if (!bonus_str.empty())
      {
        params.push_back("bonusIDs=" + bonus_str);
      }

      s << "<a style=\"color:" << item_quality_color(*item) << ";\" href=\"https://" << decoration_domain(*item->sim)
        << ".wowhead.com/item=" << item->parsed.data.id;

      if (params.size() > 0)
      {
        s << "?";
        for (size_t i = 0; i < params.size(); ++i)
        {
          s << params[i];

          if (i < params.size() - 1)
          {
            s << "&";
          }
        }
      }

      s << "\">" << util::encode_html(item->full_name()) << "</a>";
    }

    return s.str();
  }

  std::string decorate_html_string(const std::string& value, const color::rgb& color)
  {
    std::stringstream s;

    s << "<span style=\"color:" << color;
    s << "\">" << value << "</span>";

    return s.str();
  }


  std::string decorated_buff(const buff_t& buff)
  {
    return buff_decorator_t(buff).decorate();
  }

  std::string decorated_action(const action_t& a)
  {
    return action_decorator_t(a).decorate();
  }

  std::string decorated_spell_data(const sim_t& sim, const spell_data_t* spell)
  {
    return spell_data_decorator_t(&sim, spell).decorate();
  }

  std::string decorated_spell_data_item(const sim_t& sim, const spell_data_t* spell, const item_t& item)
  {
    auto decorator = spell_data_decorator_t(&sim, spell);
    decorator.item(item);
    return decorator.decorate();
  }

  std::string decorated_item(const item_t& item)
  {
    return item_decorator_t(&item).decorate();
  }
} // report_decorators